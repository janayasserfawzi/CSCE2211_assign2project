#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

int splitCount = 0;

class BTreeNode {
public:
    int  *keys;       // Array of keys
    int   t;          // Minimum degree (t=2 for 2-3-4 tree)
    BTreeNode **children; // Array of child pointers
    int   n;          // Current number of keys
    bool  leaf;       // True if leaf node

    BTreeNode(int _t, bool _leaf);
    ~BTreeNode();

    void insertNonFull(int k);
    void splitChild(int i, BTreeNode *y);
    void traverse();
    bool search(int k);

    // Level-order helper: fill an array of node-pointer arrays per level
    // (implemented in BTree via queue-based approach)

    // Serialization helpers
    void saveNode(ofstream &out);

    friend class BTree;
};

//Constructor
BTreeNode::BTreeNode(int _t, bool _leaf) {
    t    = _t;
    leaf = _leaf;
    n    = 0;
    // A node can hold at most 2t-1 keys and 2t children
    keys     = new int[2 * t - 1];
    children = new BTreeNode*[2 * t];
    for (int i = 0; i < 2 * t; i++) children[i] = nullptr;
}

//Destructor
BTreeNode::~BTreeNode() {
    for (int i = 0; i <= n; i++)
        if (children[i]) delete children[i];
    delete[] keys;
    delete[] children;
}

//traverse (in-order print)
void BTreeNode::traverse() {
    int i;
    for (i = 0; i < n; i++) {
        if (!leaf) children[i]->traverse();
        cout << keys[i] << " ";
    }
    if (!leaf) children[i]->traverse();
}

//search
bool BTreeNode::search(int k) {
    int i = 0;
    while (i < n && k > keys[i]) i++;
    if (i < n && keys[i] == k) return true;
    if (leaf) return false;
    return children[i]->search(k);
}

// Split the full child y (children[i]) of this node.
// y must have exactly 2t-1 keys before calling.
void BTreeNode::splitChild(int i, BTreeNode *y) {
    splitCount++;   // track every split

    // Create new node that will hold the right half of y
    BTreeNode *z = new BTreeNode(y->t, y->leaf);
    z->n = t - 1;

    // Copy the last (t-1) keys of y into z
    for (int j = 0; j < t - 1; j++)
        z->keys[j] = y->keys[j + t];

    // Copy the last t children of y into z (if y is not leaf)
    if (!y->leaf) {
        for (int j = 0; j < t; j++)
            z->children[j] = y->children[j + t];
    }

    // Reduce the number of keys in y
    y->n = t - 1;

    // Make room for z in this node's children array
    for (int j = n; j >= i + 1; j--)
        children[j + 1] = children[j];
    children[i + 1] = z;

    // Move the middle key of y up into this node
    for (int j = n - 1; j >= i; j--)
        keys[j + 1] = keys[j];
    keys[i] = y->keys[t - 1];

    n++;
}

//insertNonFull
void BTreeNode::insertNonFull(int k) {
    int i = n - 1;

    if (leaf) {
        // Shift keys to make room and insert
        while (i >= 0 && keys[i] > k) {
            keys[i + 1] = keys[i];
            i--;
        }
        keys[i + 1] = k;
        n++;
    } else {
        // Find the child that should receive k
        while (i >= 0 && keys[i] > k) i--;
        i++;

        // If the child is full, split it first
        if (children[i]->n == 2 * t - 1) {
            splitChild(i, children[i]);
            // After split, decide which of the two halves gets k
            if (keys[i] < k) i++;
        }
        children[i]->insertNonFull(k);
    }
}

//saveNode (preorder serialization)
void BTreeNode::saveNode(ofstream &out) {
    out << n << " " << (leaf ? 1 : 0) << " ";
    for (int i = 0; i < n; i++) out << keys[i] << " ";
    out << "\n";
    if (!leaf)
        for (int i = 0; i <= n; i++)
            children[i]->saveNode(out);
}

class BTree {
public:
    BTreeNode *root;
    int t;

    BTree(int _t) : t(_t), root(nullptr) {}
    ~BTree() { if (root) delete root; }

    void insert(int k);
    bool search(int k);
    void traverse();
    void levelOrder();

    // Persistence
    void save(const char *filename);
    void restore(const char *filename);
};

// insert 
void BTree::insert(int k) {
    if (!root) {
        root = new BTreeNode(t, true);
        root->keys[0] = k;
        root->n = 1;
        return;
    }

    if (root->n == 2 * t - 1) {
        // Root is full → create new root and split old root
        BTreeNode *s = new BTreeNode(t, false);
        s->children[0] = root;
        s->splitChild(0, root);

        // Decide which child of s gets the new key
        int i = (s->keys[0] < k) ? 1 : 0;
        s->children[i]->insertNonFull(k);

        root = s;
    } else {
        root->insertNonFull(k);
    }
}

// search 
bool BTree::search(int k) {
    if (!root) return false;
    return root->search(k);
}

// traverse
void BTree::traverse() {
    if (root) { root->traverse(); cout << "\n"; }
    else       cout << "(empty)\n";
}

//levelOrder
// Prints each level on one line: Level N: [k1 k2] [k3] ...
void BTree::levelOrder() {
    if (!root) { cout << "(empty)\n"; return; }

    // Simple queue using a fixed-size array (no STL)
    const int MAXQ = 10000;
    BTreeNode *queue[MAXQ];
    int       level_size[MAXQ]; // how many nodes in this level
    int front = 0, back = 0;

    queue[back++] = root;
    int currentLevelCount = 1;
    int levelNum = 0;

    while (front < back) {
        int nextLevelCount = 0;
        cout << "Level " << levelNum << ": ";

        for (int k = 0; k < currentLevelCount && front < back; k++) {
            BTreeNode *node = queue[front++];
            cout << "[";
            for (int i = 0; i < node->n; i++) {
                cout << node->keys[i];
                if (i < node->n - 1) cout << " ";
            }
            cout << "] ";

            if (!node->leaf) {
                for (int i = 0; i <= node->n; i++) {
                    if (node->children[i]) {
                        queue[back++] = node->children[i];
                        nextLevelCount++;
                    }
                }
            }
        }
        cout << "\n";
        currentLevelCount = nextLevelCount;
        levelNum++;
    }
}

//  save
void BTree::save(const char *filename) {
    ofstream out(filename);
    if (!out) { cerr << "Cannot open " << filename << "\n"; return; }
    out << t << "\n";
    if (root) root->saveNode(out);
    else out << "0 1\n"; // empty tree marker
    out.close();
    cout << "Tree saved to " << filename << "\n";
}

//  restore 
// Preorder rebuild: each line = "n leaf k0 k1 ... k(n-1)"
static BTreeNode* restoreNode(ifstream &in, int t) {
    int n, isLeaf;
    if (!(in >> n >> isLeaf)) return nullptr;

    BTreeNode *node = new BTreeNode(t, isLeaf == 1);
    node->n = n;
    for (int i = 0; i < n; i++) in >> node->keys[i];

    if (!isLeaf)
        for (int i = 0; i <= n; i++)
            node->children[i] = restoreNode(in, t);
    return node;
}

void BTree::restore(const char *filename) {
    ifstream in(filename);
    if (!in) { cerr << "Cannot open " << filename << "\n"; return; }

    if (root) { delete root; root = nullptr; }

    in >> t;
    root = restoreNode(in, t);
    in.close();
    cout << "Tree restored from " << filename << "\n";
}


void logState(ofstream &log, BTree &tree, const char *opDesc) {
    log << "=== " << opDesc << " ===\n";

    if (!tree.root) { log << "(empty)\n"; log << "\n"; return; }

    // Same levelOrder logic but writing to log file
    const int MAXQ = 10000;
    BTreeNode *queue[MAXQ];
    int front = 0, back = 0;
    queue[back++] = tree.root;
    int currentLevelCount = 1, levelNum = 0;

    while (front < back) {
        int nextLevelCount = 0;
        log << "Level " << levelNum << ": ";
        for (int k = 0; k < currentLevelCount && front < back; k++) {
            BTreeNode *node = queue[front++];
            log << "[";
            for (int i = 0; i < node->n; i++) {
                log << node->keys[i];
                if (i < node->n - 1) log << " ";
            }
            log << "] ";
            if (!node->leaf)
                for (int i = 0; i <= node->n; i++)
                    if (node->children[i]) { queue[back++] = node->children[i]; nextLevelCount++; }
        }
        log << "\n";
        currentLevelCount = nextLevelCount;
        levelNum++;
    }
    log << "Splits so far: " << splitCount << "\n\n";
}


int main() {
    // t=2 → 2-3-4 tree (max 3 keys, 4 children per node)
    BTree tree(2);

    ofstream log("log.txt");
    ofstream out("output.txt");

    if (!log) { cerr << "Cannot open log.txt\n"; return 1; }
    if (!out)  { cerr << "Cannot open output.txt\n"; return 1; }

    ifstream input("input.txt");
    if (!input) { cerr << "Cannot open input.txt\n"; return 1; }

    char line[256];
    while (input.getline(line, 256)) {
        // Skip empty or comment lines
        if (line[0] == '\0' || line[0] == '#') continue;

        char op;
        int  val;

        // SAVE / RESTORE 
        if (strncmp(line, "SAVE", 4) == 0) {
            tree.save("snapshot.dat");
            logState(log, tree, "SAVE");
            out << "SAVE\n";
            tree.levelOrder();
            continue;
        }
        if (strncmp(line, "RESTORE", 7) == 0) {
            tree.restore("snapshot.dat");
            out << "After RESTORE:\n";
            cout << "After RESTORE:\n";
            tree.levelOrder();
            logState(log, tree, "RESTORE");
            continue;
        }
        if (strncmp(line, "RESET", 5) == 0) {
            if (tree.root) {
            delete tree.root;
            tree.root = nullptr;
        }
        splitCount = 0;
        cout << "Tree cleared and split count reset.\n";
        continue;
        }
        //I / D / S 
        if (sscanf(line, " %c %d", &op, &val) != 2) {
            cerr << "Skipping invalid line: " << line << "\n";
            continue;
        }

        char desc[64];

        if (op == 'I' || op == 'i') {
            tree.insert(val);
            snprintf(desc, sizeof(desc), "Insert %d", val);
            cout << desc << ":\n";
            out  << desc << ":\n";
            tree.levelOrder();

            // Also write to output.txt (reuse levelOrder to stdout; duplicate logic for file)
            // Write levelOrder into output.txt
            {
                if (tree.root) {
                    const int MAXQ = 10000;
                    BTreeNode *q[MAXQ]; int fr=0,bk=0;
                    q[bk++]=tree.root; int clc=1,ln=0;
                    while(fr<bk){
                        int nlc=0; out<<"Level "<<ln<<": ";
                        for(int k=0;k<clc&&fr<bk;k++){
                            BTreeNode *nd=q[fr++]; out<<"[";
                            for(int i=0;i<nd->n;i++){out<<nd->keys[i];if(i<nd->n-1)out<<" ";}
                            out<<"] ";
                            if(!nd->leaf)for(int i=0;i<=nd->n;i++)if(nd->children[i]){q[bk++]=nd->children[i];nlc++;}
                        }
                        out<<"\n"; clc=nlc; ln++;
                    }
                }
            }
            out << "\n";
            logState(log, tree, desc);

        } else if (op == 'S' || op == 's') {
            bool found = tree.search(val);
            snprintf(desc, sizeof(desc), "Search %d", val);
            cout << desc << ": " << (found ? "FOUND" : "NOT FOUND") << "\n";
            out  << desc << ": " << (found ? "FOUND" : "NOT FOUND") << "\n\n";
            log  << " " << desc << " → " << (found ? "FOUND" : "NOT FOUND") << "\n\n";

        } else if (op == 'D' || op == 'd') {
            // Delete is optional — print a notice
            snprintf(desc, sizeof(desc), "Delete %d", val);
            cout << desc << ": Delete not implemented (optional)\n";
            out  << desc << ": Delete not implemented (optional)\n\n";
            log  << " " << desc << " skipped\n\n";

        } else {
            cerr << "Unknown operation '" << op << "' — skipping.\n";
        }
    }

    input.close();

    cout << "\nFinal Tree (In-Order)\n";
    tree.traverse();
    cout << "\nFinal Tree (Level-Order)\n";
    tree.levelOrder();
    cout << "\nTotal splits = " << splitCount << "\n";

    out  << "\nFinal Tree (Level-Order)\n";
    log  << "FINAL STATE\n";
    logState(log, tree, "Final");
    log  << "Total splits = " << splitCount << "\n";

    log.close();
    out.close();
    return 0;
}