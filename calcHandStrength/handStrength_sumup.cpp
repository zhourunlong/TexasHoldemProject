#include <bits/stdc++.h>
#include "handStrength_utils.cpp"

// generate a table for pre-flop hand strength
double handStrength[52][52];

int main() {
    for (int x = 0; x < 6; ++x) {
        char fileName[20];
        sprintf(fileName, "preflopTable_part%d", x);
        freopen(fileName, "r", stdin);
        int y, z;
        while (~scanf("%d%d", &y, &z))
            scanf("%lf", &handStrength[y][z]);
        fclose(stdin);
    }

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
}
