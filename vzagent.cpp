#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include "game.h"
#include "rng.h"
#include "net.h"
#include "./calcHandStrength/handStrength_utils.cpp"
#include "./calcHandStrength/preflopTable.cpp"

// probability to change a fold to a call, when safe
const double foldToSafeCall = 0.8;

// threshold of raise amount in each round
const int raiseAmountThreshold[4] = {1200, 4000, 10000, 15000};

// probability to give up a risky raise
const double riskyRaise = 0.5;

// threshold of call amount in each round
const int callAmountThreshold[4] = {800, 3000, 7000, 12000};

// probability to give up a risky raise
const double riskyCall = 0.3;

int preHand = -1, preRound, totalAmount[4];
double HS, fHS, relativeRank;

Action act(Game *game, MatchState *state, rng_state_t *rng) {
    Action action;
    fprintf(stderr, "********** a new action **********\n");

    // retrive private info
    int playerId = state->viewingPlayer;
    State privateState = state->state;
    int curRound = privateState.round;
    fprintf(stderr, "%dth hand, round %d out of %d\n", privateState.handId, curRound, game->numRounds);
    int spent = privateState.spent[playerId];
    int remainMoney = game->stack[playerId] - spent;

    if (privateState.handId != preHand) {
        preHand = privateState.handId;
        preRound = -1;
        memset(totalAmount, 0, sizeof(totalAmount));
    }

    char output[999];
    printMatchState(game, state, 999, output);
    fprintf(stderr, "%s\n", output);

    std::vector<int> cards;
    for (int i = 0; i < 2; ++i)
        cards.push_back(privateState.holeCards[playerId][i]);

    int publicCardNum;
    switch (curRound) {
        case 0: publicCardNum = 0; break;
        case 1: publicCardNum = 3; break;
        case 2: publicCardNum = 4; break;
        case 3: publicCardNum = 5; break;
    }

    for (int i = 0; i < publicCardNum; ++i)
        cards.push_back(privateState.boardCards[i]);

    // pot & big blind
    int pot = 0, bigBlind = 0;
    for (int i = 0; i < game->numPlayers; ++i) {
        pot += privateState.spent[i];
        bigBlind = std::max(bigBlind, game->blind[i]);
    }
    fprintf(stderr, "pot = %d\nbig blind = %d\n", pot, bigBlind);

    int raiseMin, raiseMax;
    bool canRaise = raiseIsValid(game, &privateState, &raiseMin, &raiseMax);

    // amount to raise & call
    int raiseAmount = raiseMin - spent;
    int callAmount = privateState.maxSpent - spent;
    fprintf(stderr, "min raise amount = %d\nmin call amount = %d\n", raiseAmount, callAmount);

    // hand strength
    if (curRound != preRound) {
        preRound = curRound;
        if (curRound == 0) {
            fHS = HS = handStrength[cards[0]][cards[1]];
            relativeRank = 0;
        } else {
            HS = HandStrength(cards, relativeRank);
            //double bla;
            //fHS = FactHandStrength(cards, bla);
        }
    }
    fprintf(stderr, "expected hand strength = %.10lf, relative rank = %.10lf\n", HS, relativeRank);

    // action decision
    double pf = 0, pc = 1;
    bool bluff = false;
    if (HS < 0.5) {
        pf = 1 - 4 * pow(HS, 2);
        pc = 4 * (1 - HS / 40) * pow(HS, 2);
        bluff = true;
    } else {
        pf = 0;
        pc = 0.9 - 3.6 * pow(HS - 0.5, 2);
    }

    // decision
    double probs[NUM_ACTION_TYPES];
    double actionProbs[NUM_ACTION_TYPES];
    probs[a_fold] = pf;
    probs[a_call] = pc;
    probs[a_raise] = 1 - pf - pc;

    /* build the set of valid actions */
    double p = 0.0;
    int a;
    for (a = 0; a < NUM_ACTION_TYPES; ++a)
        actionProbs[a] = 0.0;

    /* consider fold */
    bool foldValid = false;
    action.type = a_fold;
    action.size = 0;
    if (foldValid = isValidAction(game, &privateState, 0, &action)) {
        actionProbs[a_fold] = probs[a_fold];
        p += probs[a_fold];
    }

    /* consider call */
    action.type = a_call;
    action.size = 0;
    actionProbs[a_call] = probs[a_call];
    p += probs[a_call];

    /* consider raise */
    int32_t min, max;
    if (raiseIsValid(game, &privateState, &min, &max)) {
        actionProbs[a_raise] = probs[a_raise];
        p += probs[a_raise];
        fprintf(stderr, "able to raise, range = [%d, %d]\n", min, max);
    }

    /* normalise the probabilities  */
    if (p == 0.0)
        actionProbs[a_call] = p = 1;
    for (a = 0; a < NUM_ACTION_TYPES; ++a)
        actionProbs[a] /= p;

    fprintf(stderr, "decision: %.2lf fold, %.2lf call, %.2lf raise\n", actionProbs[a_fold], actionProbs[a_call], actionProbs[a_raise]);

    /* choose one of the valid actions at random */
    p = genrand_real2(rng);
    for (a = 0; a < NUM_ACTION_TYPES - 1; ++a) {
        if (p <= actionProbs[a])
            break;
        p -= actionProbs[a];
    }

    // safe to call
    if (a == a_fold && callAmount == 0 && genrand_real2(rng) < foldToSafeCall)
        a = a_call;

    // risky to raise
    if (a == a_raise) {
        double raiseRatio = 1.0 * raiseAmount / spent;
        int raiseThreshold = raiseAmountThreshold[curRound] * exp(HS - 1);
        if ((raiseAmount + totalAmount[curRound] > raiseThreshold && HS < 0.8) || raiseRatio > 10) {
            fprintf(stderr, "risky raise! ");
            double prob1 = (1.0 * (raiseAmount + totalAmount[curRound]) / raiseThreshold - 1) / HS;
            if ((curRound >= 1 && genrand_real2(rng) < std::max(riskyRaise, prob1))
              || (curRound == 0 && HS < 0.7)) {
                a = a_call;
                fprintf(stderr, "-> call");
            }
            fprintf(stderr, "\n");
        }
    }

    // risky to call
    if (a == a_call) {
        double callRatio = 1.0 * callAmount / spent;
        int callThreshold = callAmountThreshold[curRound] * exp(HS - 1);
        if ((callAmount + totalAmount[curRound] > callThreshold && HS < 0.8) || callRatio > 8) {
            fprintf(stderr, "risky call! ");
            double prob2 = (1.0 * (callAmount + totalAmount[curRound]) / callThreshold - 1) / HS;
            if (((curRound >= 1 && genrand_real2(rng) < std::max(riskyCall, prob2))
              || (curRound == 0 && HS < 0.7))
              && foldValid) {
                a = a_fold;
                fprintf(stderr, "-> fold");
            }
            fprintf(stderr, "\n");
        }
    }

    action.type = (enum ActionType)a;
    if (a == a_raise) {
        if (bluff) { // all in!
            action.size = (min + max) / 2;
        } else {
            int limit = std::max(0, raiseAmountThreshold[curRound] - totalAmount[curRound] - min);
            limit = std::min(limit, max - min);
            action.size = min + pow(HS, 10) * limit;
            totalAmount[curRound] += action.size - spent;
        }
    } else {
        action.size = 0;
        if (a == a_call)
            totalAmount[curRound] += callAmount;
    }

    fprintf(stderr, "----------  ----------  ----------\n\n");
    return action;
}


/* Anything related with socket is handled below. */
/* If you are not interested with protocol details, you can safely skip these. */

int step(int len, char line[], Game *game, MatchState *state, rng_state_t *rng) {
  /* add a colon (guaranteed to fit because we read a new-line in fgets) */
  line[ len ] = ':';
  ++len;

  Action action = act(game, state, rng);

  /* do the action! */
  assert( isValidAction( game, &(state->state), 0, &action ) );
  int r = printAction( game, &action, MAX_LINE_LEN - len - 2, &line[ len ] );
  if( r < 0 ) {
    fprintf( stderr, "ERROR: line too long after printing action\n" );
    exit( EXIT_FAILURE );
  }
  len += r;
  line[ len ] = '\r';
  ++len;
  line[ len ] = '\n';
  ++len;

  return len;
}


int main( int argc, char **argv )
{
  int sock, len;
  uint16_t port;
  Game *game;
  MatchState state;
  FILE *file, *toServer, *fromServer;
  struct timeval tv;
  rng_state_t rng;
  char line[ MAX_LINE_LEN ];

  /* we make some assumptions about the actions - check them here */
  assert( NUM_ACTION_TYPES == 3 );

  if( argc < 3 ) {

    fprintf( stderr, "usage: player server port\n" );
    exit( EXIT_FAILURE );
  }

  /* Initialize the player's random number state using time */
  gettimeofday( &tv, NULL );
  init_genrand( &rng, tv.tv_usec );

  /* get the game */
  file = fopen( "./holdem.nolimit.2p.reverse_blinds.game", "r" );
  if( file == NULL ) {

    fprintf( stderr, "ERROR: could not open game './holdem.nolimit.2p.reverse_blind.game'\n");
    exit( EXIT_FAILURE );
  }
  game = readGame( file );
  if( game == NULL ) {

    fprintf( stderr, "ERROR: could not read game './holdem.nolimit.2p.reverse_blind.game'\n");
    exit( EXIT_FAILURE );
  }
  fclose( file );

  /* connect to the dealer */
  if( sscanf( argv[ 2 ], "%"SCNu16, &port ) < 1 ) {

    fprintf( stderr, "ERROR: invalid port %s\n", argv[ 2 ] );
    exit( EXIT_FAILURE );
  }
  sock = connectTo( argv[ 1 ], port );
  if( sock < 0 ) {

    exit( EXIT_FAILURE );
  }
  toServer = fdopen( sock, "w" );
  fromServer = fdopen( sock, "r" );
  if( toServer == NULL || fromServer == NULL ) {

    fprintf( stderr, "ERROR: could not get socket streams\n" );
    exit( EXIT_FAILURE );
  }

  /* send version string to dealer */
  if( fprintf( toServer, "VERSION:%"PRIu32".%"PRIu32".%"PRIu32"\n",
	       VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION ) != 14 ) {

    fprintf( stderr, "ERROR: could not get send version to server\n" );
    exit( EXIT_FAILURE );
  }
  fflush( toServer );

  /* play the game! */
  while( fgets( line, MAX_LINE_LEN, fromServer ) ) {

    /* ignore comments */
    if( line[ 0 ] == '#' || line[ 0 ] == ';' ) {
      continue;
    }

    len = readMatchState( line, game, &state );
    if( len < 0 ) {

      fprintf( stderr, "ERROR: could not read state %s", line );
      exit( EXIT_FAILURE );
    }

    if( stateFinished( &state.state ) ) {
      /* ignore the game over message */

      continue;
    }

    if( currentPlayer( game, &state.state ) != state.viewingPlayer ) {
      /* we're not acting */

      continue;
    }

    len = step(len, line, game, &state, &rng);

    if( fwrite( line, 1, len, toServer ) != len ) {

      fprintf( stderr, "ERROR: could not get send response to server\n" );
      exit( EXIT_FAILURE );
    }
    fflush( toServer );
  }

  return EXIT_SUCCESS;
}
