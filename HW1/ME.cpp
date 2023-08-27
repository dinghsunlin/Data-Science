#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

struct NODE
{
    int item = -1, frequency = 0;
    NODE * parent;
    list<NODE *> children;

    NODE()
        {}

    NODE(const int & item, NODE * parent)
        : item(item), parent(parent) {frequency++;}

    NODE(const int & item, NODE * parent, const int & frequency)
        : item(item), parent(parent), frequency(frequency) {}

    void clean()
    {
        for(auto & i : children)
        {
            if(i->children.empty())
                delete i;
            else
                i->clean();
        }
    }
};

struct HEADERTABLE
{
    vector<int> indexTable, frequencyTable;
    vector<list<NODE *>> pointerTable;
};

void readDB(const string & inputFile,
            vector<unordered_set<int>> & DB,
            vector<int> & count)
{
    fstream input;
    input.open(inputFile, ios::in);
    DB.reserve(100000);
    string tmp;
    while(getline(input, tmp))
    {
        unordered_set<int> data;
        stringstream ss(tmp);
        string tmp2;
        while(getline(ss, tmp2, ','))
        {
            int item = stoi(tmp2);
            data.emplace(item);
            count[item]++;
        }
        DB.emplace_back(data);
    }
    input.close();
    DB.shrink_to_fit();

    for(auto & i : DB)
    {
        for(auto & j : i)
            cout << j << " ";
        cout << endl;
    }
    int a = 0;
    for(auto & i : count)
        cout << a++ << " " << i << "|";
}

void constructTable(const vector<unordered_set<int>> & DB,
                    const vector<int> & count,
                    const int & minSupport,
                    HEADERTABLE & headerTable,
                    vector<vector<int>> & newDB)
{
    vector<int> & indexTable = headerTable.indexTable;
    vector<int> & frequencyTable = headerTable.frequencyTable;
    vector<list<NODE *>> & pointerTable = headerTable.pointerTable;
    indexTable.reserve(1000);
    frequencyTable.reserve(1000);
    for(int i = 0; i < count.size(); i++)
    {
        if(count[i] >= minSupport)
        {
            indexTable.emplace_back(i);
            if(frequencyTable.size() <= i)
                frequencyTable.resize(i + 1, -1);
            frequencyTable[i] = count[i];
            cout << i << " " << count[i] << "|";
        }
    }
    indexTable.shrink_to_fit();
    frequencyTable.shrink_to_fit();
    cout << endl;
    for(int i = 0; i < indexTable.size(); i++)
        cout << i << " " << indexTable[i] << "|";
    cout << endl;
    for(int i = 0; i < frequencyTable.size(); i++)
        cout << i << " " << frequencyTable[i] << "|";
    cout << endl;
    sort(indexTable.begin(), indexTable.end(),
         [&frequencyTable](const int & a, const int & b){return frequencyTable[a] > frequencyTable[b];});
    pointerTable.resize(frequencyTable.size());
    cout << endl;
    for(int i = 0; i < indexTable.size(); i++)
        cout << i << " " << indexTable[i] << "|";
    cout << endl;
    for(int i = 0; i < frequencyTable.size(); i++)
        cout << i << " " << frequencyTable[i] << "|";
    cout << endl;
    newDB.reserve(DB.size());
    for(const auto & i : DB)
    {
        vector<int> items;
        items.reserve(indexTable.size());
        for(const auto & j : indexTable)
        {
            if(i.find(j) != i.end())
                items.emplace_back(j);
        }
        items.shrink_to_fit();
        newDB.emplace_back(items);
    }
    for(auto & i : newDB)
    {
        for(auto & j : i)
            cout << j << " ";
        cout << endl;
    }
}

NODE * FPTree(HEADERTABLE & headerTable,
                const vector<vector<int>> & newDB)
{
    const vector<int> & indexTable = headerTable.indexTable;
    const vector<int> & frequencyTable = headerTable.frequencyTable;
    vector<list<NODE *>> & pointerTable = headerTable.pointerTable;
    NODE * root = new NODE;
    for(const auto & i : newDB)
    {
        NODE * parent = root;
        for(const auto & j : i)
        {
            list<NODE *>::iterator iter = pointerTable[j].begin();
            while(iter != pointerTable[j].end())
            {
                if((*iter)->parent == parent)
                {
                    (*iter)->frequency++;
                    parent = (*iter);
                    cout << (*iter)->parent->item << " " << (*iter)->item << " " << (*iter)->frequency << endl;
                    break;
                }
                else
                    iter++;
            }
            if(iter == pointerTable[j].end())
            {
                NODE * now = new NODE(j, parent);
                parent->children.emplace_back(now);
                pointerTable[j].emplace_back(now);
                cout << "parent:" << now->parent->item << " now:" << now->item << endl;
                parent = now;
            }
        }
        cout << "---finish one transaction---\n";
    }
    for(auto & i : pointerTable)
    {
        for(auto & j : i)
        {
            cout << j->item << " " << j->parent->item << " " << j->frequency << " " << j->children.size() << endl;
        }
        if(!i.empty())
            cout << "~\n";
    }

    return root;
}

void FPGrowth(fstream & output,
              const int & minSupport,
              const HEADERTABLE & headerTable,
              const int & DBSize,
              const vector<int> & suffix,
              const bool & singlePath = false)
{
    const vector<int> & indexTable = headerTable.indexTable;
    const vector<int> & frequencyTable = headerTable.frequencyTable;
    const vector<list<NODE *>> & pointerTable = headerTable.pointerTable;

    if(singlePath)
    {
        cout << "--- Single Path ---\n";
        for(int i = 1; i <= indexTable.size(); i++)
        {
            vector<int> perturb(i, 1);
            perturb.resize(indexTable.size(), 0);
            do
            {
                bool first = true;
                for(int j = 0; j < perturb.size(); j++)
                {
                    if(perturb[j] == 1)
                    {
                        if(first)
                            output << indexTable[j];
                        else
                            output << "," << indexTable[j];
                        first = false;
                    }
                }
                for(const auto & j : suffix)
                    output << "," << j;
                output << ":" << fixed << setprecision(4) << (double)frequencyTable.back()/DBSize << endl;
            }
            while(prev_permutation(perturb.begin(), perturb.end()));
        }
    }
    else
    {
        for(int i = indexTable.size() - 1; i >= 0; i--)
        {
            cout << "--- Conditional on " << i << ": " << indexTable[i] << " ---\n";
            const int item = indexTable[i];
            vector<int> newSuffix (1, item);
            newSuffix.reserve(suffix.size() + 1);
            output << item;
            for(const auto & j : suffix)
            {
                output << "," << j;
                newSuffix.emplace_back(j);
            }
            output << ":" << fixed << setprecision(4) << (double)frequencyTable[item]/DBSize << endl;
            
            if(i == 0)
                break;

            vector<vector<int>> itemsList;
            vector<int> itemsFrequency;
            unordered_map<int, int> frequencyCount;
            for(const auto & j : pointerTable[item])
            {
                NODE * now = j->parent;
                if(now->item == -1)
                    continue;
                vector<int> items;
                items.reserve(i + 1);
                itemsFrequency.emplace_back(j->frequency);
                while(now->item != -1)
                {
                    if(frequencyCount.find(now->item) != frequencyCount.end())
                        frequencyCount[now->item] += j->frequency;
                    else
                        frequencyCount[now->item] = j->frequency;
                    items.emplace_back(now->item);
                    now = now->parent;
                }
                reverse(items.begin(), items.end());
                itemsList.emplace_back(items);
            }
            for(auto & j : itemsList)
            {
                for(auto & k : j)
                    cout << k << " ";
                cout << endl;
            }
            for(auto & j : frequencyCount)
                cout << j.first << ":" << j.second << "|";
            cout << endl;
            HEADERTABLE newHeaderTable;
            vector<int> & newIndexTable = newHeaderTable.indexTable;
            vector<int> & newFrequencyTable = newHeaderTable.frequencyTable;
            vector<list<NODE *>> & newPointerTable = newHeaderTable.pointerTable;
            newIndexTable.reserve(indexTable.size());
            for(auto j = frequencyCount.begin(); j != frequencyCount.end(); )
            {
                if(j->second < minSupport)
                    j = frequencyCount.erase(j);
                else
                {
                    newIndexTable.emplace_back(j->first);
                    if(newFrequencyTable.size() <= j->first)
                        newFrequencyTable.resize(j->first + 1, -1);
                    newFrequencyTable[j->first] = j->second;
                    j++;
                }
            }
            for(auto & j : frequencyCount)
                cout << j.first << ":" << j.second << "|";
            cout << endl;
            if(frequencyCount.empty())
                continue;
            newIndexTable.shrink_to_fit();
            newFrequencyTable.shrink_to_fit();
            sort(newIndexTable.begin(), newIndexTable.end(),
                 [&newFrequencyTable](const int & a, const int & b){return newFrequencyTable[a] > newFrequencyTable[b];});
            for(int j = 0; j < newIndexTable.size(); j++)
                cout << j << " " << newIndexTable[j] << "|";
            cout << endl;
            for(int j = 0; j < newFrequencyTable.size(); j++)
                cout << j << " " << newFrequencyTable[j] << "|";
            cout << endl;
            newPointerTable.resize(newFrequencyTable.size());
            NODE * root = new NODE;
            bool firstList = true;
            bool newSinglePath = true;
            for(int j = 0; j < itemsList.size(); j++)
            {
                NODE * parent = root;
                for(const auto & k : itemsList[j])
                {
                    if(frequencyCount.find(k) == frequencyCount.end())
                        continue;
                    cout << k << " ";
                    list<NODE *>::iterator iter = newPointerTable[k].begin();
                    while(iter != newPointerTable[k].end())
                    {
                        if((*iter)->parent == parent)
                        {
                            (*iter)->frequency += itemsFrequency[j];
                            parent = (*iter);
                            cout << (*iter)->parent->item << " " << (*iter)->item << " " << (*iter)->frequency << endl;
                            break;
                        }
                        else
                            iter++;
                    }
                    if(iter == newPointerTable[k].end())
                    {
                        if(!firstList)
                            newSinglePath = false;
                        NODE * now = new NODE(k, parent, itemsFrequency[j]);
                        parent->children.emplace_back(now);
                        newPointerTable[k].emplace_back(now);
                        cout << "parent:" << now->parent->item << " now:" << now->item << endl;
                        parent = now;
                    }
                }
                firstList = false;
            }
            if(!root->children.empty())
                FPGrowth(output, minSupport, newHeaderTable, DBSize, newSuffix, newSinglePath);
            root->clean();
            delete root;
        }
        // int a;
        // cin >> a;
    }
}

// void FPGrowth(const string & outputFile,
//               const int & minSupport,
//               const HEADERTABLE & headerTable,
//               const int & DBSize)
// {
//     fstream output;
//     output.open(outputFile, ios::out);
//     const vector<int> & indexTable = headerTable.indexTable;
//     const vector<int> & frequencyTable = headerTable.frequencyTable;
//     const vector<list<NODE *>> & pointerTable = headerTable.pointerTable;

//     for(int i = indexTable.size() - 1; i >= 0; i--)
//     {
//         const int item = indexTable[i];
//         output << item << ":" << fixed << setprecision(4) << (double)frequencyTable[item]/DBSize << endl;
//         if(i == 0)
//             break;

//         vector<vector<int>> itemsList;
//         vector<int> itemsFrequency;
//         unordered_map<int, int> frequencyCount;
//         for(const auto & j : pointerTable[item])
//         {
//             NODE * now = j->parent;
//             if(now->item == -1)
//                 continue;
//             vector<int> items;
//             items.reserve(i + 1);
//             itemsFrequency.emplace_back(j->frequency);
//             while(now->item != -1)
//             {
//                 if(frequencyCount.find(now->item) != frequencyCount.end())
//                     frequencyCount[now->item] += j->frequency;
//                 else
//                     frequencyCount[now->item] = j->frequency;
//                 items.emplace_back(now->item);
//                 now = now->parent;
//             }
//             reverse(items.begin(), items.end());
//             itemsList.emplace_back(items);
//         }
//         unordered_set<int> removedItems;
//         for(auto j = frequencyCount.begin(); j != frequencyCount.end(); )
//         {
//             if(j->second < minSupport)
//             {
//                 removedItems.emplace(j->first);
//                 j = frequencyCount.erase(j);
//             }
//             else
//                 j++;
//         }
//         unordered_map<int, vector<int>> sizeGroup;
//         int count = 0;
//         auto f = itemsFrequency.begin();
//         for(auto j = itemsList.begin();  j != itemsList.end(); )
//         {
//             for(auto k = (*j).begin(); k != (*j).end(); )
//             {
//                 if(removedItems.find(*k) != removedItems.end())
//                     k = (*j).erase(k);
//                 else
//                     k++;
//             }
//             if((*j).empty())
//             {
//                 j = itemsList.erase(j);
//                 f = itemsFrequency.erase(f);
//             }
//             else
//             {
//                 sizeGroup[(int)(*j).size()].emplace_back(count++);
//                 j++;
//                 f++;
//             }
//         }
//         for(auto & j : sizeGroup)
//         {
//             int visited = 0, left = j.second.size();
//             for(int k = 0; k < j.second.size() - 1; k++)
//             {
//                 if(j.second[k] == -1)
//                     continue;
//                 for(int l = k + 1; l < j.second.size(); l++)
//                 {
//                     if(itemsList[j.second[k]] == itemsList[j.second[l]])
//                     {
//                         left--;
//                         itemsList[j.second[l]].clear();
//                         itemsFrequency[j.second[k]] += itemsFrequency[j.second[l]];
//                         itemsFrequency[j.second[l]] = 0;
//                         j.second[l] = -1;
//                     }
//                 }
//                 visited++;
//                 if(visited >= left)
//                     break;
//             }
//         }
//         map<vector<int>, int> outputBuffer;
//         for(int j = 0; j < itemsList.size(); j++)
//         {
//             if(itemsList[j].empty())
//                 continue;
//             for(int k = 1; k <= itemsList[j].size(); k++)
//             {
//                 vector<int>  perturb(k, 1);
//                 perturb.resize(itemsList[j].size(), 0);
//                 do
//                 {
//                     vector<int> list;
//                     list.reserve(k);
//                     for(int l = 0; l < perturb.size(); l++)
//                     {
//                         if(perturb[l] == 1)
//                             list.emplace_back(itemsList[j][l]);
//                     }
//                     if(outputBuffer.find(list) != outputBuffer.end())
//                         outputBuffer[list] += itemsFrequency[j];
//                     else
//                         outputBuffer[list] = itemsFrequency[j];
//                 }
//                 while(prev_permutation(perturb.begin(), perturb.end()));
//             }
//         }
//         for(auto & j : outputBuffer)
//         {
//             if(j.second < minSupport)
//                 continue;
//             output << item;
//             for(auto & k : j.first)
//                 output << "," << k;
//             output << ":" << fixed << setprecision(4) << (double)j.second/DBSize << endl;
//         }
//         cout << item << endl;
//         for(auto & j : itemsList)
//         {
//             for(auto & k : j)
//                 cout << k << " ";
//             cout << endl;
//         }
//         for(auto & j : itemsFrequency)
//             cout << j << " ";
//         cout << endl;
//         for(auto & j : frequencyCount)
//             cout << j.first << ": " << j.second << endl;
//         for(auto & j : removedItems)
//             cout << j << " ";
//         cout << endl;
//         for(auto & j : sizeGroup)
//         {
//             cout << j.first << ": [ ";
//             for(auto & k : j.second)
//                 cout << k << " ";
//             cout << "]\n";
//         }
//         for(auto & j : outputBuffer)
//         {
//             for(auto & k : j.first)
//                 cout << k << " ";
//             cout << "| " << j.second << endl;
//         }
//     }

//     output.close();
// }

int main(int argc, char * argv[])
{
    auto start = chrono::steady_clock::now();

    const double minSupportRate = atof(argv[1]);
    const string inputFile = argv[2];
    const string outputFile = argv[3];

    vector<unordered_set<int>> DB;
    vector<int> count(1000, 0);
    readDB(inputFile, DB, count);

    const int minSupport = minSupportRate * DB.size();

    cout << DB.size() << ' ' << minSupport << endl;
    
    HEADERTABLE headerTable;
    vector<vector<int>> newDB;
    constructTable(DB, count, minSupport, headerTable, newDB);

    NODE * root = FPTree(headerTable, newDB);

    fstream output;
    vector<int> suffix;
    output.open(outputFile, ios::out);
    FPGrowth(output, minSupport, headerTable, (int)newDB.size(), suffix);
    output.close();

    auto end = chrono::steady_clock::now();
    cout << "Total Runtime:\t" << chrono::duration<float>(end - start).count() << "\tsec.\n";
}