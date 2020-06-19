#include <bits/stdc++.h>
#include "handStrength_utils.cpp"

// generate a table for pre-flop hand strength
// multithread on six cores
long long cnt, tot;
int rmy;
double handStrength[52][52];
UsedArray used(52);
std::vector<int> my, opponent;

void dfs(int dep, int last) {
    if (dep == 7) {
        tot += 2;
        int rop = rankHand(opponent);
        if (rmy > rop)
            cnt += 2;
        if (rmy == rop)
            ++cnt;
        return;
    }
    if (dep < 5) {
        for (int x = used.successor(last); x < 52; x = used.successor(x)) {
            used.use(x);
            my.push_back(x);
            opponent.push_back(x);

            dfs(dep + 1, dep == 4 ? -1 : x);

            used.free(x);
            my.pop_back();
            opponent.pop_back();
        }
    } else {
        if (dep == 5)
            rmy = rankHand(my);
        for (int x = used.successor(last); x < 52; x = used.successor(x)) {
            used.use(x);
            opponent.push_back(x);

            dfs(dep + 1, x);

            used.free(x);
            opponent.pop_back();
        }
    }
}

int main(int argc, char** argv) {
    int numThread = atoi(argv[1]);
    fprintf(stderr, "now executing on thread %d\n", numThread);

    int order = 0;
    char fileName[20];
    sprintf(fileName, "preflopTable_part%d", numThread);
    freopen(fileName, "w", stdout);
    for (int i = 0; i < 13; ++i) {
        int x = makeCard(i, 0);
        used.use(x);
        my.push_back(x);
        for (int j = i; j < 13; ++j)
            for (int k = 0; k < 2; ++k) {
                if (j == i && k == 0)
                    continue;
                int y = makeCard(j, k);
                used.use(y);
                my.push_back(y);

                ++order;
                if (order % 6 == numThread) {
                    cnt = tot = 0;
                    dfs(0, -1);
                    printf("%d %d %.10lf\n", x, y, 1.0 * cnt / tot);
                    fprintf(stderr, "%d %d %.10lf\n", x, y, 1.0 * cnt / tot);
                }

                used.free(y);
                my.pop_back();
            }
        used.free(x);
        my.pop_back();
    }

    /*
    for (int i = 0; i < 13; ++i)
        for (int k = 0; k < 4; ++k) {
            int x = makeCard(i, k);
            for (int j = 0; j < 13; ++j)
                for (int l = 0; l < 4; ++l) {
                    int y = makeCard(j, l);
                    if (x == y) continue;
                    int ii = std::min(i, j), jj = std::max(i, j);
                    handStrength[x][y] = handStrength[makeCard(ii, 0)][makeCard(jj, k != l)];
                }
        }

    freopen("preflopTable.cpp", "w", stdout);
    printf("const double handStrength[52][52] = {\n");
    for (int i = 0; i < 52; ++i) {
        printf("{");
        for (int j = 0; j < 51; ++j)
            printf("%.10lf, ", handStrength[i][j]);
        printf("%.10lf}", handStrength[i][51]);
        if (i < 51)
            printf(",");
        printf("\n");
    }
    printf("};\n");
    */

    /*
    std::vector<int> cards;
    cards.push_back(makeCard(12, 0));
    cards.push_back(makeCard(11, 1));
    cards.push_back(makeCard(10, 2));
    cards.push_back(makeCard(9, 3));
    cards.push_back(makeCard(8, 0));
    printf("%.10lf %.10lf\n", MCHandStrength(cards), HandStrength(cards));
    // 0.9350000000 0.9311444286
    */
}
