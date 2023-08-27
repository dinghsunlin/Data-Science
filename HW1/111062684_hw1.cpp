#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define numThread 16

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
        }
    }
    indexTable.shrink_to_fit();
    frequencyTable.shrink_to_fit();
    sort(indexTable.begin(), indexTable.end(),
         [&frequencyTable](const int & a, const int & b){return frequencyTable[a] > frequencyTable[b];});
    pointerTable.resize(frequencyTable.size());
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
                parent = now;
            }
        }
    }

    return root;
}

void FPGrowth(fstream & output,
              const int & minSupport,
              const HEADERTABLE & headerTable,
              const int & DBSize,
              const int & shift,
              mutex & mtx)
{
    const vector<int> & indexTable = headerTable.indexTable;
    const vector<int> & frequencyTable = headerTable.frequencyTable;
    const vector<list<NODE *>> & pointerTable = headerTable.pointerTable;

    for(int i = indexTable.size() - shift; i >= 0; i -= numThread)
    {
        const int item = indexTable[i];
        mtx.lock();
        output << item << ":" << fixed << setprecision(4) << (double)frequencyTable[item]/DBSize << endl;
        mtx.unlock();
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
        unordered_set<int> removedItems;
        for(auto j = frequencyCount.begin(); j != frequencyCount.end(); )
        {
            if(j->second < minSupport)
            {
                removedItems.emplace(j->first);
                j = frequencyCount.erase(j);
            }
            else
                j++;
        }
        unordered_map<int, vector<int>> sizeGroup;
        int count = 0;
        auto f = itemsFrequency.begin();
        for(auto j = itemsList.begin();  j != itemsList.end(); )
        {
            for(auto k = (*j).begin(); k != (*j).end(); )
            {
                if(removedItems.find(*k) != removedItems.end())
                    k = (*j).erase(k);
                else
                    k++;
            }
            if((*j).empty())
            {
                j = itemsList.erase(j);
                f = itemsFrequency.erase(f);
            }
            else
            {
                sizeGroup[(int)(*j).size()].emplace_back(count++);
                j++;
                f++;
            }
        }
        for(auto & j : sizeGroup)
        {
            int visited = 0, left = j.second.size();
            for(int k = 0; k < j.second.size() - 1; k++)
            {
                if(j.second[k] == -1)
                    continue;
                for(int l = k + 1; l < j.second.size(); l++)
                {
                    if(itemsList[j.second[k]] == itemsList[j.second[l]])
                    {
                        left--;
                        itemsList[j.second[l]].clear();
                        itemsFrequency[j.second[k]] += itemsFrequency[j.second[l]];
                        itemsFrequency[j.second[l]] = 0;
                        j.second[l] = -1;
                    }
                }
                visited++;
                if(visited >= left)
                    break;
            }
        }
        map<vector<int>, int> outputBuffer;
        for(int j = 0; j < itemsList.size(); j++)
        {
            if(itemsList[j].empty())
                continue;
            for(int k = 1; k <= itemsList[j].size(); k++)
            {
                vector<int>  perturb(k, 1);
                perturb.resize(itemsList[j].size(), 0);
                do
                {
                    vector<int> list;
                    list.reserve(k);
                    for(int l = 0; l < perturb.size(); l++)
                    {
                        if(perturb[l] == 1)
                            list.emplace_back(itemsList[j][l]);
                    }
                    if(outputBuffer.find(list) != outputBuffer.end())
                        outputBuffer[list] += itemsFrequency[j];
                    else
                        outputBuffer[list] = itemsFrequency[j];
                }
                while(prev_permutation(perturb.begin(), perturb.end()));
            }
        }
        mtx.lock();
        for(auto & j : outputBuffer)
        {
            if(j.second < minSupport)
                continue;
            output << item;
            for(auto & k : j.first)
                output << "," << k;
            output << ":" << fixed << setprecision(4) << (double)j.second/DBSize << endl;
        }
        mtx.unlock();
    }
}

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
    
    HEADERTABLE headerTable;
    vector<vector<int>> newDB;
    constructTable(DB, count, minSupport, headerTable, newDB);

    NODE * root = FPTree(headerTable, newDB);

    fstream output;
    vector<thread> threads;
    mutex mtx;
    output.open(outputFile, ios::out);
    for(int i = 1; i <= numThread; i++)
        threads.emplace_back(thread(FPGrowth, ref(output), cref(minSupport), cref(headerTable), newDB.size(), i, ref(mtx)));
    for(auto & i : threads)
        i.join();
    output.close();

    auto end = chrono::steady_clock::now();
    cout << "Total Runtime:\t" << chrono::duration<float>(end - start).count() << "\tsec.\n";
}