#include <iostream>
using namespace std;

struct Node{
    int val;
    Node* next;
};

Node* func(Node* head) {
    Node* dummyHead = new Node();
    dummyHead->next = head;
    Node* cur = head;
    Node* a = dummyHead;
    while (cur) {
        if (cur && cur->next && cur->val == cur->next->val) {
            while (cur && cur->next && cur->val == cur->next->val) {
                cur = cur->next;
                // delete
                
            }
            cur = cur->next;
        }
        if (!cur || !cur->next || cur->val != cur->next->val) {
            a->next = cur;
            // cout << cur->val << " ";
            a = a->next;
            cur = cur->next;
        }
    }
    head = dummyHead->next;
    delete dummyHead;
    dummyHead = nullptr;
    return head;
}

void print(Node* head) {
    Node* cur = head;
    while (cur) {
        cout << cur->val << " ";
        cur = cur->next;
    }
    cout << endl;
}

int main() {
    Node* dummyHead = new Node();
    Node* cur = dummyHead;
    int x;
    while (cin >> x) {
        Node* nd = new Node();
        nd->val = x;
        cur->next = nd;
        cur = cur->next;
    }
    cur = dummyHead->next;
    delete dummyHead;
    dummyHead = nullptr;
    print(cur);
    cur = func(cur);
    print(cur);
    return 0;
}