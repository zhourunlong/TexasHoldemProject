#include <bits/stdc++.h>
#include "../handRank/rank_hand.cpp"

const int MC_Round = 1000;

// dancing links to optimize finding unused card
// map from 0~n-1 to 1~n
class UsedArray {
public:
    UsedArray(int _n) {
        n = _n;
        for (int i = 0; i <= n + 1; ++i) {
            pre[i] = i - 1;
            nxt[i] = i + 1;
            used[i] = false;
        }
    }
    int predecessor(int x) {
        //assert(0 <= x && x <= n);
        ++x;
        //assert(!used[x]);
        return pre[x] - 1;
    }
    int successor(int x) {
        //assert(-1 <= x && x <= n - 1);
        ++x;
        //assert(!used[x]);
        return nxt[x] - 1;
    }
    void use(int x) {
        //assert(0 <= x && x <= n - 1);
        ++x;
        //assert(!used[x]);
        used[x] = true;
        nxt[pre[x]] = nxt[x];
        pre[nxt[x]] = pre[x];
    }
    void free(int x) {
        //assert(0 <= x && x <= n - 1);
        ++x;
        //assert(used[x]);
        used[x] = false;
        nxt[pre[x]] = pre[nxt[x]] = x;
    }

private:
    bool used[60];
    int n, pre[60], nxt[60];
};

// 0~3 -> cdhs
char ToSuit(int t) {
    assert(0 <= t && t <= 3);
    switch (t) {
        case 0: return 'c';
        case 1: return 'd';
        case 2: return 'h';
        case 3: return 's';
    }
}

// 0~12 -> 2~A
char ToRank(int t) {
    assert(0 <= t && t <= 12);
    if (t <= 7) return '2' + t;
    switch (t) {
        case 8: return 'T';
        case 9: return 'J';
        case 10: return 'Q';
        case 11: return 'K';
        case 12: return 'A';
    }
}

// using Monte Carlo to calculate hand strength
// input size: 2, 5, 6, 7
// my: first two cards
// public: 3rd ~ 7th cards
double MCHandStrength(std::vector<int> cards) {
    int n = cards.size();
    assert(n == 2 || n == 5 || n == 6 || n == 7);

    bool used[52];
    memset(used, 0, sizeof(used));
    for (int i = 0; i < n; ++i)
        used[cards[i]] = true;
    std::vector<int> my, opponent;
    for (int i = 0; i < n; ++i)
        my.push_back(cards[i]);
    for (int i = 2; i < n; ++i)
        opponent.push_back(cards[i]);

    double cnt = 0;
    for (int round = 0; round < MC_Round; ++round) {
        for (int x = n; x < 7; ++x) {
            int y = rand() % 52;
            while (used[y])
                y = rand() % 52;
            my.push_back(y);
            opponent.push_back(y);
            used[y] = true;
        }
        for (int x = 0; x < 2; ++x) {
            int y = rand() % 52;
            while (used[y])
                y = rand() % 52;
            opponent.push_back(y);
            used[y] = true;
        }

        int rmy = rankHand(my), rop = rankHand(opponent);
        if (rmy > rop)
            ++cnt;
        if (rmy == rop)
            cnt += 0.5;

        for (int x = n; x < 9; ++x) {
            used[*(opponent.rbegin())] = false;
            opponent.pop_back();
        }
        for (int x = n; x < 7; ++x)
            my.pop_back();
    }
    return cnt / MC_Round;
}

// input size: 5, 6, 7
// my: first two cards
// public: 3rd ~ 7th cards
double HandStrength(std::vector<int> cards) {
    int n = cards.size();
    assert(n == 5 || n == 6 || n == 7);

    UsedArray used(52);
    for (int i = 0; i < n; ++i)
        used.use(cards[i]);
    std::vector<int> my, opponent;
    for (int i = 0; i < n; ++i)
        my.push_back(cards[i]);
    for (int i = 2; i < n; ++i)
        opponent.push_back(cards[i]);

    long long cnt = 0, tot = 0;
    switch (n) {
        case 5:
            for (int y6 = used.successor(-1); y6 < 52; y6 = used.successor(y6)) {
                used.use(y6);
                my.push_back(y6);
                opponent.push_back(y6);

                for (int y7 = used.successor(-1); y7 < 52; y7 = used.successor(y7)) {
                    used.use(y7);
                    my.push_back(y7);
                    opponent.push_back(y7);
                    int rmy = rankHand(my);

                    for (int y8 = used.successor(-1); y8 < 52; y8 = used.successor(y8)) {
                        used.use(y8);
                        opponent.push_back(y8);

                        for (int y9 = used.successor(-1); y9 < 52; y9 = used.successor(y9)) {
                            opponent.push_back(y9);

                            tot += 2;
                            int rop = rankHand(opponent);
                            if (rmy > rop)
                                cnt += 2;
                            if (rmy == rop)
                                ++cnt;

                            opponent.pop_back();
                        }

                        used.free(y8);
                        opponent.pop_back();
                    }

                    used.free(y7);
                    my.pop_back();
                    opponent.pop_back();
                }

                used.free(y6);
                my.pop_back();
                opponent.pop_back();
            }
        case 6:
            for (int y7 = used.successor(-1); y7 < 52; y7 = used.successor(y7)) {
                used.use(y7);
                my.push_back(y7);
                opponent.push_back(y7);
                int rmy = rankHand(my);

                for (int y8 = used.successor(-1); y8 < 52; y8 = used.successor(y8)) {
                    used.use(y8);
                    opponent.push_back(y8);

                    for (int y9 = used.successor(-1); y9 < 52; y9 = used.successor(y9)) {
                        opponent.push_back(y9);

                        tot += 2;
                        int rop = rankHand(opponent);
                        if (rmy > rop)
                            cnt += 2;
                        if (rmy == rop)
                            ++cnt;

                        opponent.pop_back();
                    }

                    used.free(y8);
                    opponent.pop_back();
                }

                used.free(y7);
                my.pop_back();
                opponent.pop_back();
            }
        case 7:
            int rmy = rankHand(my);
            for (int y8 = used.successor(-1); y8 < 52; y8 = used.successor(y8)) {
                used.use(y8);
                opponent.push_back(y8);

                for (int y9 = used.successor(-1); y9 < 52; y9 = used.successor(y9)) {
                    opponent.push_back(y9);

                    tot += 2;
                    int rop = rankHand(opponent);
                    if (rmy > rop)
                        cnt += 2;
                    if (rmy == rop)
                        ++cnt;

                    opponent.pop_back();
                }

                used.free(y8);
                opponent.pop_back();
            }
    }
    return 1.0 * cnt / tot;
}
