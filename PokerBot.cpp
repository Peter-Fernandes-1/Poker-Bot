#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>

// Constants
const int DECK_SIZE = 52;
const int SIMULATION_TIME_LIMIT_MS = 10000; 
const double WIN_PROBABILITY_THRESHOLD = 0.5; 
const double UCB1_CONSTANT = 1.41421356237; 


// Card suits
enum Suit {
    CLUBS = 0,
    DIAMONDS,
    HEARTS,
    SPADES
};


// Card values
enum Value {
    TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN,
    JACK, QUEEN, KING, ACE
};



// Hand rankings
enum HandRank {
    HIGH_CARD = 0,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH,
    ROYAL_FLUSH
};

// Card representation
struct Card {
    Suit suit;
    Value value;
    
    Card(Suit s, Value v) : suit(s), value(v) {}
    
    // Convert card to integer for efficient comparison (0-51)
    int toInt() const {
        return static_cast<int>(suit) * 13 + static_cast<int>(value) - 2;
    }
    
    static Card fromInt(int cardIndex) {
        int suitIndex = cardIndex / 13;
        int valueIndex = cardIndex % 13 + 2;
        return Card(static_cast<Suit>(suitIndex), static_cast<Value>(valueIndex));
    }
    
    // For printing cards
    std::string toString() const {
        std::string valueStr;
        switch (value) {
            case TWO: valueStr = "2"; break;
            case THREE: valueStr = "3"; break;
            case FOUR: valueStr = "4"; break;
            case FIVE: valueStr = "5"; break;
            case SIX: valueStr = "6"; break;
            case SEVEN: valueStr = "7"; break;
            case EIGHT: valueStr = "8"; break;
            case NINE: valueStr = "9"; break;
            case TEN: valueStr = "10"; break;
            case JACK: valueStr = "J"; break;
            case QUEEN: valueStr = "Q"; break;
            case KING: valueStr = "K"; break;
            case ACE: valueStr = "A"; break;
            default: valueStr = "?";
        }
        
        char suitChar;
        switch (suit) {
            case CLUBS: suitChar = 'C'; break;
            case DIAMONDS: suitChar = 'D'; break;
            case HEARTS: suitChar = 'H'; break;
            case SPADES: suitChar = 'S'; break;
            default: suitChar = '?';
        }
        
        return valueStr + suitChar;
    }
    
    bool operator==(const Card& other) const {
        return suit == other.suit && value == other.value;
    }
};

// Function to check if two cards are equal (used for find operations)
bool cardsEqual(const Card& a, const Card& b) {
    return a.toInt() == b.toInt();
}

class Deck {
private:
    std::vector<Card> cards;

public:
    Deck() {
        reset();
    }
    
    void reset() {
        cards.clear();
        for (int s = 0; s < 4; s++) {
            for (int v = 2; v <= 14; v++) {
                cards.push_back(Card(static_cast<Suit>(s), static_cast<Value>(v)));
            }
        }
    }
    


    void shuffle() {
        // Using Fisher-Yates shuffle
        for (size_t i = cards.size() - 1; i > 0; --i) {
            size_t j = std::rand() % (i + 1);
            Card temp = cards[i];
            cards[i] = cards[j];
            cards[j] = temp;
        }
    }
    
    Card deal() {
        if (cards.empty()) {
            throw std::runtime_error("No cards left in the deck");
        }
        
        Card card = cards.back();
        cards.pop_back();
        return card;
    }
    
    void removeCard(const Card& card) {
        for (size_t i = 0; i < cards.size(); ++i) {
            if (cardsEqual(cards[i], card)) {
                cards.erase(cards.begin() + i);
                return;
            }
        }
    }
    
    int cardsLeft() const {
        return cards.size();
    }

    // Create a specific card and remove it from the deck
    Card createCard(Suit suit, Value value) {
        Card card(suit, value);
        removeCard(card);
        return card;
    }
};


// Represents a complete hand evaluation
struct HandEvaluation {
    HandRank rank;
    std::vector<int> tiebreakers; 
    
    bool operator>(const HandEvaluation& other) const {
        if (rank != other.rank) {
            return rank > other.rank;
        }
        
        for (size_t i = 0; i < tiebreakers.size() && i < other.tiebreakers.size(); i++) {
            if (tiebreakers[i] != other.tiebreakers[i]) {
                return tiebreakers[i] > other.tiebreakers[i];
            }
        }
        
        return false;
    }
    
    bool operator==(const HandEvaluation& other) const {
        if (rank != other.rank) {
            return false;
        }
        
        if (tiebreakers.size() != other.tiebreakers.size()) {
            return false;
        }
        
        for (size_t i = 0; i < tiebreakers.size(); i++) {
            if (tiebreakers[i] != other.tiebreakers[i]) {
                return false;
            }
        }
        
        return true;
    }
};

class HandEvaluator {
public:
    // Evaluate a poker hand (2 hole cards + up to 5 community cards)
    static HandEvaluation evaluate(const std::vector<Card>& cards) {
        // If we don't have exactly 7 cards, create a full hand with random cards
        if (cards.size() != 7) {

            Deck tempDeck;
            for (size_t i = 0; i < cards.size(); ++i) {
                tempDeck.removeCard(cards[i]);
            }
            
            std::vector<Card> fullHand;
            for (size_t i = 0; i < cards.size(); ++i) {
                fullHand.push_back(cards[i]);
            }
            
            tempDeck.shuffle();
            while (fullHand.size() < 7) {
                fullHand.push_back(tempDeck.deal());
            }
            
            return evaluateComplete(fullHand);
        }
        
        return evaluateComplete(cards);
    }
    
    // Evaluate a complete 7-card hand (MUST have exactly 7 cards)
    static HandEvaluation evaluateComplete(const std::vector<Card>& cards) {

        std::vector<int> valueCounts(15, 0); 
        for (size_t i = 0; i < cards.size(); ++i) {
            const Card& card = cards[i];
            valueCounts[static_cast<int>(card.value)]++;
        }
        
        // Count frequencies of each suit
        std::vector<int> suitCounts(4, 0);
        for (size_t i = 0; i < cards.size(); ++i) {
            const Card& card = cards[i];
            suitCounts[static_cast<int>(card.suit)]++;
        }
        
        // Check for flush
        bool hasFlush = false;
        Suit flushSuit = CLUBS; 
        for (int i = 0; i < 4; i++) {
            if (suitCounts[i] >= 5) {
                hasFlush = true;
                flushSuit = static_cast<Suit>(i);
                break;
            }
        }
        
        // Check for straight
        bool hasStraight = false;
        int straightHighCard = 0;
        
        // Special case for Ace-low straight (A-2-3-4-5)
        if (valueCounts[14] > 0 && valueCounts[2] > 0 && valueCounts[3] > 0 && 
            valueCounts[4] > 0 && valueCounts[5] > 0) {
            hasStraight = true;
            straightHighCard = 5; 
        }
        
        // Check for other straights
        for (int i = 10; i >= 2; i--) {
            if (valueCounts[i] > 0 && valueCounts[i+1] > 0 && valueCounts[i+2] > 0 && 
                valueCounts[i+3] > 0 && valueCounts[i+4] > 0) {
                hasStraight = true;
                straightHighCard = i + 4;
                break;
            }
        }
        
        // Extract cards of the flush suit
        std::vector<Card> flushCards;
        if (hasFlush) {
            for (size_t i = 0; i < cards.size(); ++i) {
                const Card& card = cards[i];
                if (card.suit == flushSuit) {
                    flushCards.push_back(card);
                }
            }
            
            // Sort flush cards by value (descending)
            std::sort(flushCards.begin(), flushCards.end(), compareCardsByValueDesc);
        }
        
        // Check for straight flush
        bool hasStraightFlush = false;
        int straightFlushHighCard = 0;
        
        if (hasFlush && flushCards.size() >= 5) {
            if (hasStraight && straightHighCard == 14) {
                bool isRoyal = true;
                for (int v = 10; v <= 14; v++) {
                    bool found = false;
                    for (size_t i = 0; i < flushCards.size(); ++i) {
                        const Card& card = flushCards[i];
                        if (static_cast<int>(card.value) == v) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        isRoyal = false;
                        break;
                    }
                }
                
                if (isRoyal) {
                    hasStraightFlush = true;
                    straightFlushHighCard = 14;
                }
            }
            
            // Check for other straight flushes
            if (!hasStraightFlush) {

                std::vector<int> flushValues;
                for (size_t i = 0; i < flushCards.size(); ++i) {
                    const Card& card = flushCards[i];
                    flushValues.push_back(static_cast<int>(card.value));
                }
                
                // Sort and remove duplicates
                std::sort(flushValues.begin(), flushValues.end(), std::greater<int>());
                flushValues.erase(std::unique(flushValues.begin(), flushValues.end()), flushValues.end());
                
                // Check for A-5 straight flush
                if (std::find(flushValues.begin(), flushValues.end(), 14) != flushValues.end() &&
                    std::find(flushValues.begin(), flushValues.end(), 2) != flushValues.end() &&
                    std::find(flushValues.begin(), flushValues.end(), 3) != flushValues.end() &&
                    std::find(flushValues.begin(), flushValues.end(), 4) != flushValues.end() &&
                    std::find(flushValues.begin(), flushValues.end(), 5) != flushValues.end()) {
                    hasStraightFlush = true;
                    straightFlushHighCard = 5;
                }
                
                // Check for regular straight flushes
                for (size_t i = 0; i <= flushValues.size() - 5; i++) {
                    if (flushValues[i] == flushValues[i+4] + 4) {
                        hasStraightFlush = true;
                        straightFlushHighCard = flushValues[i];
                        break;
                    }
                }
            }
        }
        
        // Find sets (pairs, trips, quads)
        std::vector<int> quads;
        std::vector<int> trips;
        std::vector<int> pairs;
        
        for (int i = 2; i <= 14; i++) {
            if (valueCounts[i] == 4) {
                quads.push_back(i);
            } else if (valueCounts[i] == 3) {
                trips.push_back(i);
            } else if (valueCounts[i] == 2) {
                pairs.push_back(i);
            }
        }
        
        // Sort in descending order
        std::sort(quads.begin(), quads.end(), std::greater<int>());
        std::sort(trips.begin(), trips.end(), std::greater<int>());
        std::sort(pairs.begin(), pairs.end(), std::greater<int>());
        
        // Get high cards (for kickers)
        std::vector<int> highCards;
        for (int i = 14; i >= 2; i--) {
            if (valueCounts[i] == 1) {
                highCards.push_back(i);
            }
        }
        
        // Determine hand rank and tiebreakers
        HandEvaluation evaluation;
        
        // Royal Flush
        if (hasStraightFlush && straightFlushHighCard == 14) {
            evaluation.rank = ROYAL_FLUSH;
            evaluation.tiebreakers.push_back(14); 
        }
        // Straight Flush
        else if (hasStraightFlush) {
            evaluation.rank = STRAIGHT_FLUSH;
            evaluation.tiebreakers.push_back(straightFlushHighCard);
        }
        // Four of a Kind
        else if (!quads.empty()) {
            evaluation.rank = FOUR_OF_A_KIND;
            evaluation.tiebreakers.push_back(quads[0]);
            
            // Add highest kicker
            for (int i = 14; i >= 2; i--) {
                if (valueCounts[i] > 0 && i != quads[0]) {
                    evaluation.tiebreakers.push_back(i);
                    break;
                }
            }
        }
        // Full House
        else if (!trips.empty() && (!pairs.empty() || trips.size() > 1)) {
            evaluation.rank = FULL_HOUSE;
            
            // Highest three of a kind
            evaluation.tiebreakers.push_back(trips[0]);
            
            // Highest pair (or second trips)
            if (!pairs.empty()) {
                evaluation.tiebreakers.push_back(pairs[0]);
            } else {
                evaluation.tiebreakers.push_back(trips[1]);
            }
        }
        // Flush
        else if (hasFlush) {
            evaluation.rank = FLUSH;
            
            // Take top 5 cards of flush suit
            for (size_t i = 0; i < 5 && i < flushCards.size(); i++) {
                evaluation.tiebreakers.push_back(static_cast<int>(flushCards[i].value));
            }
        }
        // Straight
        else if (hasStraight) {
            evaluation.rank = STRAIGHT;
            evaluation.tiebreakers.push_back(straightHighCard);
        }
        // Three of a Kind
        else if (!trips.empty()) {
            evaluation.rank = THREE_OF_A_KIND;
            evaluation.tiebreakers.push_back(trips[0]);
            
            // Add top two kickers
            int kickerCount = 0;
            for (size_t i = 0; i < highCards.size() && kickerCount < 2; ++i) {
                int highCard = highCards[i];
                evaluation.tiebreakers.push_back(highCard);
                kickerCount++;
            }
        }
        // Two Pair
        else if (pairs.size() >= 2) {
            evaluation.rank = TWO_PAIR;
            evaluation.tiebreakers.push_back(pairs[0]);
            evaluation.tiebreakers.push_back(pairs[1]);
            
            // Add highest kicker
            if (!highCards.empty()) {
                evaluation.tiebreakers.push_back(highCards[0]);
            }
        }
        // One Pair
        else if (pairs.size() == 1) {
            evaluation.rank = PAIR;
            evaluation.tiebreakers.push_back(pairs[0]);
            
            // Add top three kickers
            int kickerCount = 0;
            for (size_t i = 0; i < highCards.size() && kickerCount < 3; ++i) {
                int highCard = highCards[i];
                evaluation.tiebreakers.push_back(highCard);
                kickerCount++;
            }
        }
        // High Card
        else {
            evaluation.rank = HIGH_CARD;
            
            // Add top five cards
            int cardCount = 0;
            for (int i = 14; i >= 2 && cardCount < 5; i--) {
                if (valueCounts[i] > 0) {
                    evaluation.tiebreakers.push_back(i);
                    cardCount++;
                }
            }
        }
        
        return evaluation;
    }
    
    // Helper function for sorting cards by value in descending order
    static bool compareCardsByValueDesc(const Card& a, const Card& b) {
        return static_cast<int>(a.value) > static_cast<int>(b.value);
    }
    
    static std::string handRankToString(HandRank rank) {
        switch (rank) {
            case HIGH_CARD: return "High Card";
            case PAIR: return "Pair";
            case TWO_PAIR: return "Two Pair";
            case THREE_OF_A_KIND: return "Three of a Kind";
            case STRAIGHT: return "Straight";
            case FLUSH: return "Flush";
            case FULL_HOUSE: return "Full House";
            case FOUR_OF_A_KIND: return "Four of a Kind";
            case STRAIGHT_FLUSH: return "Straight Flush";
            case ROYAL_FLUSH: return "Royal Flush";
            default: return "Unknown";
        }
    }
};

// Poker game simulator
class PokerGame {
private:
    Deck deck;
    std::vector<Card> botHoleCards;
    std::vector<Card> opponentHoleCards;
    std::vector<Card> communityCards;
    
public:
    // Initialize a new game
    void initialize() {
        deck.reset();
        deck.shuffle();
        
        botHoleCards.clear();
        opponentHoleCards.clear();
        communityCards.clear();
        
        // Deal hole cards
        botHoleCards.push_back(deck.deal());
        botHoleCards.push_back(deck.deal());
        
        opponentHoleCards.push_back(deck.deal());
        opponentHoleCards.push_back(deck.deal());
    }
    
    // Initialize with known bot cards and community cards (for simulation)
    void initialize(const std::vector<Card>& knownBotCards, const std::vector<Card>& knownCommunityCards) {
        deck.reset();
        
        botHoleCards.clear();
        for (size_t i = 0; i < knownBotCards.size(); ++i) {
            botHoleCards.push_back(knownBotCards[i]);
        }
        
        communityCards.clear();
        for (size_t i = 0; i < knownCommunityCards.size(); ++i) {
            communityCards.push_back(knownCommunityCards[i]);
        }
        
        // Remove known cards from the deck
        for (size_t i = 0; i < botHoleCards.size(); ++i) {
            deck.removeCard(botHoleCards[i]);
        }
        
        for (size_t i = 0; i < communityCards.size(); ++i) {
            deck.removeCard(communityCards[i]);
        }
        
        deck.shuffle();
        
        // For simulation: deal random opponent cards
        opponentHoleCards.clear();
        opponentHoleCards.push_back(deck.deal());
        opponentHoleCards.push_back(deck.deal());
    }
    
    // Deal the flop (3 cards)
    void dealFlop() {
        if (communityCards.size() >= 3) {
            return; // Flop already dealt
        }
        
        while (communityCards.size() < 3) {
            communityCards.push_back(deck.deal());
        }
    }
    
    // Deal the turn (4th card)
    void dealTurn() {
        if (communityCards.size() >= 4) {
            return; // Turn already dealt
        }
        
        if (communityCards.size() < 3) {
            dealFlop();
        }
        
        communityCards.push_back(deck.deal());
    }
    
    // Deal the river (5th card)
    void dealRiver() {
        if (communityCards.size() >= 5) {
            return; // River already dealt
        }
        
        if (communityCards.size() < 4) {
            dealTurn();
        }
        
        communityCards.push_back(deck.deal());
    }
    
    // Complete the board by dealing any remaining community cards
    void completeBoard() {
        dealRiver();
    }
    
    // Determine the winner (true if bot wins, false if opponent wins)
    bool isWinner() {
        // Make sure all cards are dealt
        completeBoard();
        
        // Combine hole cards with community cards
        std::vector<Card> botHand;
        for (size_t i = 0; i < botHoleCards.size(); ++i) {
            botHand.push_back(botHoleCards[i]);
        }
        for (size_t i = 0; i < communityCards.size(); ++i) {
            botHand.push_back(communityCards[i]);
        }
        
        std::vector<Card> opponentHand;
        for (size_t i = 0; i < opponentHoleCards.size(); ++i) {
            opponentHand.push_back(opponentHoleCards[i]);
        }
        for (size_t i = 0; i < communityCards.size(); ++i) {
            opponentHand.push_back(communityCards[i]);
        }
        
        // Evaluate hands
        HandEvaluation botEval = HandEvaluator::evaluate(botHand);
        HandEvaluation opponentEval = HandEvaluator::evaluate(opponentHand);
        
        // Compare hands (positive result means bot wins)
        return (botEval > opponentEval);
    }
    
    // Print current game state
    void printGameState(bool showOpponentCards = false) {
        std::cout << "Bot hole cards: ";
        for (size_t i = 0; i < botHoleCards.size(); ++i) {
            std::cout << botHoleCards[i].toString() << " ";
        }
        std::cout << std::endl;
        
        if (showOpponentCards) {
            std::cout << "Opponent hole cards: ";
            for (size_t i = 0; i < opponentHoleCards.size(); ++i) {
                std::cout << opponentHoleCards[i].toString() << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Opponent cards: [hidden]" << std::endl;
        }
        
        std::cout << "Community cards: ";
        if (communityCards.empty()) {
            std::cout << "[none yet]";
        } else {
            for (size_t i = 0; i < communityCards.size(); ++i) {
                std::cout << communityCards[i].toString() << " ";
            }
        }
        std::cout << std::endl;
    }
};

// MCTS node for poker decisions
class MCTSNode {
private:
    int wins;
    int visits;
    double explorationParameter;
    
public:
    MCTSNode() : wins(0), visits(0), explorationParameter(UCB1_CONSTANT) {}
    
    void update(bool isWin) {
        visits++;
        if (isWin) {
            wins++;
        }
    }
    
    double getWinProbability() const {
        if (visits == 0) return 0.0;
        return static_cast<double>(wins) / visits;
    }
    
    int getVisits() const {
        return visits;
    }
    
    int getWins() const {
        return wins;
    }
    
    double getUCB1Value(int parentVisits) const {
        if (visits == 0) {
            return std::numeric_limits<double>::infinity();
        }
        
        double exploitation = static_cast<double>(wins) / visits;
        double exploration = explorationParameter * std::sqrt(std::log(parentVisits) / visits);
        
        return exploitation + exploration;
    }
};

// Monte Carlo Tree Search Poker Bot
class PokerBot {
private:
    PokerGame game;
    std::vector<Card> myCards;
    std::vector<Card> community;
    
    // Keep track of runs for statistics
    int totalRuns;
    int winningRuns;
    MCTSNode rootNode;
    
public:
    PokerBot() : totalRuns(0), winningRuns(0) {
        // Seed the random number generator
        std::srand(static_cast<unsigned int>(std::time(NULL)));
    }
    
    // Set the bot's hole cards and any known community cards
    void setKnownCards(const std::vector<Card>& holeCards, const std::vector<Card>& communityCards) {
        myCards = holeCards;
        community = communityCards;
    }
    
    // Get the bot's hole cards
    const std::vector<Card>& getHoleCards() const {
        return myCards;
    }
    
    // Get the community cards
    const std::vector<Card>& getCommunityCards() const {
        return community;
    }
    
    // Run Monte Carlo simulations for a specified time limit
    double runMCTS(int msTimeLimit) {
        clock_t startTime = clock();
        totalRuns = 0;
        winningRuns = 0;
        
        // Create root node
        rootNode = MCTSNode();
        
        while (true) {
            // Check time limit (approximate conversion to milliseconds)
            clock_t currentTime = clock();
            int elapsedMs = ((currentTime - startTime) * 1000) / CLOCKS_PER_SEC;
                
            if (elapsedMs >= msTimeLimit) {
                break;
            }
            
            // Run a single simulation
            bool won = runSingleSimulation();
            rootNode.update(won);
            
            totalRuns++;
            if (won) {
                winningRuns++;
            }
        }
        
        return getWinProbability();
    }
    
    // Run a single MCTS simulation
    bool runSingleSimulation() {
        // Initialize game with known cards
        game.initialize(myCards, community);
        
        // Complete the deal
        game.completeBoard();
        
        // Determine winner
        return game.isWinner();
    }
    
    // Get current win probability estimate
    double getWinProbability() const {
        return rootNode.getWinProbability();
    }
    
    // Decide whether to fold or stay
    bool shouldStay() const {
        double winProbability = getWinProbability();
        return winProbability >= WIN_PROBABILITY_THRESHOLD;
    }
    
    // Print statistics
    void printStats() const {
        std::cout << "Simulations run: " << totalRuns << std::endl;
        std::cout << "Wins: " << winningRuns << std::endl;
        std::cout << "Win probability: " << std::fixed << std::setprecision(2) 
                  << (getWinProbability() * 100.0) << "%" << std::endl;
        std::cout << "Decision: " << (shouldStay() ? "STAY" : "FOLD") << std::endl;
    }
};

// For testing purposes
void testHandEvaluator() {
    std::cout << "Testing Hand Evaluator..." << std::endl;
    
    // Create test hands
    Deck deck;
    
    // Royal Flush
    std::vector<Card> royalFlush;
    royalFlush.push_back(Card(HEARTS, ACE));
    royalFlush.push_back(Card(HEARTS, KING));
    royalFlush.push_back(Card(HEARTS, QUEEN));
    royalFlush.push_back(Card(HEARTS, JACK));
    royalFlush.push_back(Card(HEARTS, TEN));
    royalFlush.push_back(Card(CLUBS, TWO));
    royalFlush.push_back(Card(DIAMONDS, THREE));
    
    // Straight Flush
    std::vector<Card> straightFlush;
    straightFlush.push_back(Card(SPADES, NINE));
    straightFlush.push_back(Card(SPADES, EIGHT));
    straightFlush.push_back(Card(SPADES, SEVEN));
    straightFlush.push_back(Card(SPADES, SIX));
    straightFlush.push_back(Card(SPADES, FIVE));
    straightFlush.push_back(Card(CLUBS, TWO));
    straightFlush.push_back(Card(DIAMONDS, THREE));
    
    // Four of a Kind
    std::vector<Card> fourOfAKind;
    fourOfAKind.push_back(Card(HEARTS, ACE));
    fourOfAKind.push_back(Card(DIAMONDS, ACE));
    fourOfAKind.push_back(Card(CLUBS, ACE));
    fourOfAKind.push_back(Card(SPADES, ACE));
    fourOfAKind.push_back(Card(HEARTS, KING));
    fourOfAKind.push_back(Card(CLUBS, TWO));
    fourOfAKind.push_back(Card(DIAMONDS, THREE));
    
    // Test each hand
    HandEvaluation eval1 = HandEvaluator::evaluate(royalFlush);
    HandEvaluation eval2 = HandEvaluator::evaluate(straightFlush);
    HandEvaluation eval3 = HandEvaluator::evaluate(fourOfAKind);
    
    // Print results
    std::cout << "Royal Flush: " << HandEvaluator::handRankToString(eval1.rank) << std::endl;
    std::cout << "Straight Flush: " << HandEvaluator::handRankToString(eval2.rank) << std::endl;
    std::cout << "Four of a Kind: " << HandEvaluator::handRankToString(eval3.rank) << std::endl;
    
    // Test comparison
    std::cout << "Royal Flush > Straight Flush: " << (eval1 > eval2 ? "Yes" : "No") << std::endl;
    std::cout << "Straight Flush > Four of a Kind: " << (eval2 > eval3 ? "Yes" : "No") << std::endl;
    
    std::cout << "Hand Evaluator Tests Complete" << std::endl;
}

// Parse a card string (e.g., "AS" for Ace of Spades)
Card parseCard(const std::string& cardStr) {
    if (cardStr.size() < 2) {
        throw std::runtime_error("Invalid card format");
    }
    
    // Parse value
    Value value;
    if (cardStr[0] == 'A') value = ACE;
    else if (cardStr[0] == 'K') value = KING;
    else if (cardStr[0] == 'Q') value = QUEEN;
    else if (cardStr[0] == 'J') value = JACK;
    else if (cardStr[0] == 'T' || cardStr[0] == '1') value = TEN;
    else if (cardStr[0] >= '2' && cardStr[0] <= '9') {
        value = static_cast<Value>(cardStr[0] - '0');
    } else {
        throw std::runtime_error("Invalid card value");
    }
    
    // Parse suit
    Suit suit;
    char suitChar = cardStr.back();
    if (suitChar == 'C' || suitChar == 'c') suit = CLUBS;
    else if (suitChar == 'D' || suitChar == 'd') suit = DIAMONDS;
    else if (suitChar == 'H' || suitChar == 'h') suit = HEARTS;
    else if (suitChar == 'S' || suitChar == 's') suit = SPADES;
    else {
        throw std::runtime_error("Invalid card suit");
    }
    
    return Card(suit, value);
}

// Main function for running the bot
int main(int argc, char* argv[]) {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    
    // Uncomment to run hand evaluator tests
    // testHandEvaluator();
    
    // Create the poker bot
    PokerBot bot;
    
    // Game phase tracking
    int phase = 0; // 0 = pre-flop, 1 = pre-turn, 2 = pre-river
    
    std::cout << "MCTS Poker Bot" << std::endl;
    std::cout << "--------------" << std::endl;
    
    // Main game loop
    while (phase <= 2) {
        std::vector<Card> holeCards;
        std::vector<Card> communityCards;
        
        // Get bot's hole cards (for real game, input would come from the game server)
        if (phase == 0) {
            // Example: Get hole cards from user input
            std::cout << "Enter bot's two hole cards (e.g., AS KH for Ace of Spades and King of Hearts): ";
            std::string card1, card2;
            std::cin >> card1 >> card2;
            
            try {
                holeCards.push_back(parseCard(card1));
                holeCards.push_back(parseCard(card2));
            } catch (const std::runtime_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                continue;
            }
        } else {
            // For phases after pre-flop, we need to reuse the hole cards from the previous phase
            holeCards = bot.getHoleCards();  // Get the hole cards stored in the bot
            communityCards = bot.getCommunityCards();  // Get existing community cards
        }
        
        // Get community cards based on phase
        if (phase >= 1) {  // After flop
            if (phase == 1 && communityCards.empty()) {
                // Get flop cards
                std::cout << "Enter the three flop cards (e.g., 2C 7H QS): ";
                std::string card1, card2, card3;
                std::cin >> card1 >> card2 >> card3;
                
                try {
                    communityCards.push_back(parseCard(card1));
                    communityCards.push_back(parseCard(card2));
                    communityCards.push_back(parseCard(card3));
                } catch (const std::runtime_error& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    continue;
                }
            }
        }
        
        if (phase >= 2) {  // After turn
            if (phase == 2 && communityCards.size() == 3) {
                // Get turn card
                std::cout << "Enter the turn card (e.g., 5D): ";
                std::string turnCard;
                std::cin >> turnCard;
                
                try {
                    communityCards.push_back(parseCard(turnCard));
                } catch (const std::runtime_error& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                    continue;
                }
            }
        }
        
        // Display current game state
        std::cout << "\nCurrent Game State:" << std::endl;
        std::cout << "----------------" << std::endl;
        std::cout << "Your hole cards: ";
        for (size_t i = 0; i < holeCards.size(); ++i) {
            std::cout << holeCards[i].toString() << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Community cards: ";
        for (size_t i = 0; i < communityCards.size(); ++i) {
            std::cout << communityCards[i].toString() << " ";
        }
        std::cout << std::endl;
        
        // Set the bot's known cards
        bot.setKnownCards(holeCards, communityCards);
        
        // Run MCTS simulations (10 seconds)
        std::cout << "\nRunning simulations (10 seconds)..." << std::endl;
        bot.runMCTS(SIMULATION_TIME_LIMIT_MS);
        
        // Display stats and decision
        std::cout << "\nSimulation Results:" << std::endl;
        std::cout << "------------------" << std::endl;
        bot.printStats();
        
        // Display current phase
        std::string phaseStr;
        switch (phase) {
            case 0: phaseStr = "Pre-Flop"; break;
            case 1: phaseStr = "Pre-Turn"; break;
            case 2: phaseStr = "Pre-River"; break;
            default: phaseStr = "Unknown";
        }
        
        std::cout << "\nCurrent phase: " << phaseStr << std::endl;
        
        // Ask to continue to next phase
        if (phase < 2) {
            std::cout << "Continue to next phase? (y/n): ";
            char choice;
            std::cin >> choice;
            
            if (choice == 'y' || choice == 'Y') {
                phase++;
            } else {
                break;
            }
        } else {
            std::cout << "Game complete." << std::endl;
            break;
        }
    }
    
    return 0;
}
