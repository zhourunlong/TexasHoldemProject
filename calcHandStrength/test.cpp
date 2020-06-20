#include <bits/stdc++.h>
#include "handStrength_utils.cpp"

int main() {
    std::vector<int> cards;
    cards.push_back(makeCard(12, 0));
    cards.push_back(makeCard(11, 1));
    cards.push_back(makeCard(10, 2));
    cards.push_back(makeCard(9, 3));
    cards.push_back(makeCard(8, 2));
    printf("%.10lf\n", MCHandStrength(cards));
    printf("%.10lf\n", HandStrength(cards));
}
