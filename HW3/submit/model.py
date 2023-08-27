import torch
import torch.nn as nn
import torch.nn.functional as F

from dgl.nn.pytorch import GraphConv, SAGEConv

class GCN(nn.Module):
    """
    Baseline Model:
    - A simple two-layer GCN model, similar to https://github.com/tkipf/pygcn
    - Implement with DGL package
    """
    def __init__(self, in_size, hid_size, out_size):
        super().__init__()
        self.layers = nn.ModuleList()
        # two-layer GCN
        self.layers.append(GraphConv(in_size, hid_size, activation=F.relu))
        self.layers.append(GraphConv(hid_size, out_size))
        self.dropout = nn.Dropout(0.5)

    def forward(self, g, features):
        h = features
        for i, layer in enumerate(self.layers):
            if i != 0:
                h = self.dropout(h)
            h = layer(g, h)
        return h
    
# class YourGNNModel(nn.Module):
#     """
#     TODO: Use GCN model as reference, implement your own model here to achieve higher accuracy on testing data
#     """
#     def __init__(self, in_size, hid_size, out_size):
#         super().__init__()
    
#     def forward(self, g, features):
#         pass

class SAGE(nn.Module):
    def __init__(self, in_size, hid_size, out_size):
        super().__init__()
        self.layers = nn.ModuleList()
        # two-layer GraphSAGE-mean
        self.layers.append(SAGEConv(in_size, hid_size, "gcn"))
        self.layers.append(SAGEConv(hid_size, out_size, "gcn"))
        self.dropout = nn.Dropout(0.5)

    def forward(self, graph, x):
        h = self.dropout(x)
        for l, layer in enumerate(self.layers):
            h = layer(graph, h)
            if l != len(self.layers) - 1:
                h = F.relu(h)
                h = self.dropout(h)
        return h
    
class GraceGCN(nn.Module):
    def __init__(self, in_dim, out_dim):
        super().__init__()
        self.layers = nn.ModuleList()
        self.layers.append(GraphConv(in_dim, out_dim * 2))
        self.layers.append(GraphConv(out_dim * 2, out_dim))

    def forward(self, graph, features):
        out = features
        for layer in self.layers:
            out = F.relu(layer(graph, out))
        return out
    
class GraceSAGE(nn.Module):
    def __init__(self, in_dim, out_dim):
        super().__init__()
        self.layers = nn.ModuleList()
        self.layers.append(SAGEConv(in_dim, out_dim * 2, "gcn"))
        self.layers.append(SAGEConv(out_dim * 2, out_dim, "gcn"))

    def forward(self, graph, features):
        out = features
        for layer in self.layers:
            out = F.relu(layer(graph, out))
        return out
    
class GraceMLP(nn.Module):
    def __init__(self, in_dim, out_dim):
        super().__init__()
        self.fc1 = nn.Linear(in_dim, out_dim)
        self.fc2 = nn.Linear(out_dim, in_dim)

    def forward(self, x):
        out = F.elu(self.fc1(x))
        return self.fc2(out)
    
class Grace(nn.Module):
    def __init__(self, in_dim, hid_dim, out_dim, temp):
        super().__init__()
        self.temp = temp
        self.encoder = GraceSAGE(in_dim, hid_dim)
        self.proj = GraceMLP(hid_dim, out_dim)

    def get_loss(self, p1, p2):
        func = lambda x: torch.exp(x / self.temp)
        ref = func(torch.mm(F.normalize(p1), F.normalize(p1).t()))
        bet = func(torch.mm(F.normalize(p1), F.normalize(p2).t()))
        loss = -torch.log(bet.diag() / (ref.sum(1) - ref.diag() + bet.sum(1)))
        return loss

    def forward(self, graph1, features1, graph2, features2):
        e1 = self.encoder(graph1, features1)
        e2 = self.encoder(graph2, features2)

        p1 = self.proj(e1)
        p2 = self.proj(e2)

        out = (self.get_loss(p1, p2) + self.get_loss(p2, p1)) * 0.5
        return out.mean()

    def get_embedding(self, graph, features):
        return self.encoder(graph, features).detach()