from argparse import ArgumentParser

from data_loader import load_data

import torch

from sklearn.ensemble import GradientBoostingClassifier
from sklearn.model_selection import GridSearchCV
from sklearn.multiclass import OneVsRestClassifier
from sklearn.preprocessing import normalize, OneHotEncoder

from model import GCN, SAGE, Grace
# from model import YourGNNModel # Build your model in model.py

import dgl
import numpy as np

import os
import warnings
warnings.filterwarnings("ignore")

def evaluate(embed, train_mask, train_labels, val_mask, val_labels):
    """Evaluate model accuracy"""
    X = embed.detach().cpu().numpy()
    X = normalize(X, norm='l2')

    OHE = OneHotEncoder(categories='auto')
    Y = train_labels.detach().cpu().numpy()
    Y = Y.reshape(-1, 1)
    Y = OHE.fit_transform(Y).toarray().astype(np.bool)

    gradboost = GradientBoostingClassifier()
    parameters = {
        "estimator__learning_rate": [0.001, 0.005, 0.01, 0.05, 0.1],
        "estimator__n_estimators": [50, 100, 500, 1000],
        "estimator__subsample": [0.5, 0.6, 0.7, 0.8, 0.9, 1],
    }

    model2 = GridSearchCV(estimator=OneVsRestClassifier(gradboost), param_grid=parameters, n_jobs=-1, cv=5, verbose=4)
    model2.fit(X[train_mask], Y)

    y_eval = model2.predict_proba(X[val_mask])
    y_eval = np.argmax(y_eval, axis=1)
    y_eval = torch.from_numpy(y_eval)
    correct = torch.sum(y_eval == val_labels)
    acc = correct.item() * 1.0 / len(val_labels)
    print(f"Accuracy {acc:.4f}")

    return model2, X, acc

def train(g, features, model, args, device):
    
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr, weight_decay=args.wd)

    # training loop
    for epoch in range(args.epochs):
        model.train()
        optimizer.zero_grad()
        graph1, features1 = aug(g, features, args.dfr1, args.dfr2)
        graph2, features2 = aug(g, features, args.dfr2, args.der2)
        loss = model(graph1.to(device), features1.to(device), graph2.to(device), features2.to(device))
        loss.backward()
        optimizer.step()

        print(
            "Epoch {:05d} | Loss {:.4f}".format(
                epoch, loss.item()
            )
        )
    
    return loss.item()

def aug(graph, features, dfr, der):
    num_nodes = graph.num_nodes()
    num_edges = graph.num_edges()

    drop_mask = (torch.empty((features.size(1),), dtype=torch.float32, device=features.device).uniform_(0, 1) < dfr)
    new_features = features.clone()
    new_features[:, drop_mask] = 0

    mask_rates = torch.FloatTensor(np.ones(num_edges) * der)
    masks = torch.bernoulli(1 - mask_rates)
    mask_idx = masks.nonzero().squeeze(1)
    new_src = graph.edges()[0][mask_idx]
    new_dst = graph.edges()[1][mask_idx]

    new_graph = dgl.graph((new_src, new_dst), num_nodes=num_nodes).add_self_loop()

    return new_graph, new_features

if __name__ == '__main__':

    parser = ArgumentParser()
    # you can add your arguments if needed
    parser.add_argument('--epochs', type=int, default=1500, help='number of training periods')
    parser.add_argument('--gpu', type=int, default=1, help='use which gpu to train')
    parser.add_argument('--lr', type=float, default=0.001, help='learning rate')
    parser.add_argument('--wd', type=float, default=1e-5, help='weight decay')
    parser.add_argument('--temp', type=float, default=1, help='temperature')
    parser.add_argument('--hid_dim', type=int, default=128)
    parser.add_argument('--out_dim', type=int, default=128)
    parser.add_argument('--dfr1', type=float, default=0.4, help='drop feature ratio of the 1st augmentation')
    parser.add_argument('--dfr2', type=float, default=0.3, help='drop feature ratio of the 1st augmentation')
    parser.add_argument('--der1', type=float, default=0.2, help='drop edge ratio of the 1st augmentation')
    parser.add_argument('--der2', type=float, default=0.1, help='drop edge ratio of the 2nd augmentation')
    
    args = parser.parse_args()

    if args.gpu == -1:
        device = torch.device("cpu")
    else:
        device = torch.device("cuda:{}".format(args.gpu) if torch.cuda.is_available() else "cpu")
        if torch.cuda.is_available():
            print(f"Running on GPU {args.gpu}")

    # Load data
    features, graph, num_classes, \
    train_labels, val_labels, test_labels, \
    train_mask, val_mask, test_mask = load_data()

    # features = features.to(device)
    # graph = graph.to(device)
    # train_labels = train_labels.to(device)
    # val_labels = val_labels.to(device)
    # test_labels = test_labels.to(device)
    # train_mask = train_mask.to(device)
    # val_mask = val_mask.to(device)
    # test_mask = test_mask.to(device)
    
    # Initialize the model (Baseline Model: GCN)
    """TODO: build your own model in model.py and replace GCN() with your model"""
    in_size = features.shape[1]
    hid_size = args.hid_dim
    out_size = args.out_dim
    model = Grace(in_size, hid_size, out_size, args.temp).to(device)
    
    # model training
    print("Training...")
    last_loss = train(graph, features, model, args, device)
    
    print("Testing...")
    graph = graph.add_self_loop()
    graph = graph.to(device)
    features = features.to(device)
    embed = model.get_embedding(graph, features)
    model2, X, acc = evaluate(embed, train_mask, train_labels, val_mask, val_labels)
    y_pred = model2.predict_proba(X[test_mask])
    y_pred = np.argmax(y_pred, axis=1)
    
    # Export predictions as csv file
    print(f"Export predictions as csv file. GPU: {args.gpu}")
    print("{} | Last Loss {:.4f} | Accuracy {:.4f}".format(args, last_loss, acc))
    with open("output{}.csv".format(args.gpu), 'w') as f:
        f.write('Id,Predict\n')
        for idx, pred in enumerate(y_pred):
            f.write(f'{idx},{int(pred)}\n')
    # Please remember to upload your output.csv file to Kaggle for scoring

    torch.cuda.empty_cache()