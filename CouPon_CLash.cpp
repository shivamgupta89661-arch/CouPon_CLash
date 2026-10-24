#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <cstdlib>

using namespace std;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

struct Date {
    int y, m, d;

    static Date today() {
        time_t t = time(nullptr);
        tm* lt = localtime(&t);
        return { lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday };
    }

    static Date fromDaysFromNow(int days) {
        time_t t = time(nullptr) + (long)days * 86400L;
        tm* lt = localtime(&t);
        return { lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday };
    }

    string toString() const {
        ostringstream o;
        o << setfill('0') << setw(4) << y << '-'
          << setw(2) << m << '-' << setw(2) << d;
        return o.str();
    }

    long toDays() const {
        int yy = y - (m <= 2);
        int era = (yy >= 0 ? yy : yy - 399) / 400;
        unsigned yoe = static_cast<unsigned>(yy - era * 400);
        unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097L + static_cast<long>(doe) - 719468L;
    }

    long daysLeftUntil(const Date& other) const { return other.toDays() - toDays(); }
};

class Coupon {
protected:
    string code;
    string brand;
    double minCart;
    Date   expiry;
    bool   used;

public:
    Coupon(string c, string b, double m, Date e)
        : code(c), brand(b), minCart(m), expiry(e), used(false) {}

    virtual ~Coupon() {}

    virtual double apply(double cart) const = 0;
    virtual string marketLabel() const = 0;
    virtual string saveLine() const = 0;

    bool isExpired() const { return daysLeft() < 0; }
    long daysLeft()  const { return Date::today().daysLeftUntil(expiry); }
    void markUsed()        { used = true; }

    bool validate(double cart) const {
        if (used)           return false;
        if (isExpired())    return false;
        if (cart < minCart) return false;
        return true;
    }

    string getCode()  const { return code;  }
    string getBrand() const { return brand; }
    double getMinCart() const { return minCart; }

    string getFullDetails() const {
        return code + " (" + brand + ") — " + marketLabel()
             + " | min cart Rs." + to_string((int)minCart)
             + " | " + to_string(daysLeft()) + " days left";
    }
};

class FlatCoupon : public Coupon {
    double discount;
public:
    FlatCoupon(string c, string b, double m, Date e, double d)
        : Coupon(c, b, m, e), discount(d) {}
    double apply(double cart) const { return discount < cart ? discount : cart; }
    string marketLabel() const { return "Flat Rs." + to_string((int)discount) + " off"; }
    string saveLine() const {
        return "FLAT," + code + "," + brand + "," +
               to_string((int)minCart) + "," +
               to_string(daysLeft()) + "," +
               to_string((int)discount);
    }
};

class PercentCoupon : public Coupon {
    double percent, cap;
public:
    PercentCoupon(string c, string b, double m, Date e, double p, double c2)
        : Coupon(c, b, m, e), percent(p), cap(c2) {}
    double apply(double cart) const {
        double val = cart * percent / 100.0;
        return val < cap ? val : cap;
    }
    string marketLabel() const {
        return to_string((int)percent) + "% off (max Rs." + to_string((int)cap) + ")";
    }
    string saveLine() const {
        return "PERCENT," + code + "," + brand + "," +
               to_string((int)minCart) + "," +
               to_string(daysLeft()) + "," +
               to_string((int)percent) + "," +
               to_string((int)cap);
    }
};

class BOGOCoupon : public Coupon {
public:
    BOGOCoupon(string c, string b, double m, Date e) : Coupon(c, b, m, e) {}
    double apply(double cart) const { return cart / 2; }
    string marketLabel() const { return "Buy 1 Get 1 Free"; }
    string saveLine() const {
        return "BOGO," + code + "," + brand + "," +
               to_string((int)minCart) + "," +
               to_string(daysLeft());
    }
};

class ReferralCoupon : public Coupon {
    double bonus;
public:
    ReferralCoupon(string c, string b, double m, Date e, double bns)
        : Coupon(c, b, m, e), bonus(bns) {}
    double apply(double cart) const { return bonus < cart ? bonus : cart; }
    string marketLabel() const { return "Referral bonus Rs." + to_string((int)bonus); }
    string saveLine() const {
        return "REFERRAL," + code + "," + brand + "," +
               to_string((int)minCart) + "," +
               to_string(daysLeft()) + "," +
               to_string((int)bonus);
    }
};

const int MAX_OWNED = 20;

class User {
    string username;
    string password;
    long   coins;
    Coupon* owned[MAX_OWNED];
    int     ownedCount;

public:
    User() : coins(0), ownedCount(0) {}
    User(string u, string p, long c) : username(u), password(p), coins(c), ownedCount(0) {}

    string getName() const { return username; }
    string getPassword() const { return password; }
    bool checkPassword(const string& p) const { return password == p; }
    long getCoins() const { return coins; }
    int  getOwnedCount() const { return ownedCount; }

    bool canAfford(long amount) const { return coins >= amount; }
    void addCoins(long amount) { coins += amount; }

    bool spendCoins(long amount) {
        if (amount > coins) return false;
        coins -= amount;
        return true;
    }

    void addCoupon(Coupon* c) {
        if (ownedCount < MAX_OWNED) owned[ownedCount++] = c;
    }

    void viewOwned() const {
        if (ownedCount == 0) { cout << "No coupons owned.\n"; return; }
        for (int i = 0; i < ownedCount; i++)
            cout << "  " << (i + 1) << ". " << owned[i]->getFullDetails() << "\n";
    }
};

struct Bid {
    string bidderName;
    long   amount;
    bool   active;

    Bid() : amount(0), active(false) {}
    Bid(string n, long a) : bidderName(n), amount(a), active(true) {}
};

const int MAX_LISTINGS = 50;

class Marketplace {
    Coupon* listings[MAX_LISTINGS];
    Bid     bids[MAX_LISTINGS];
    int     count;

    static const int MIN_DAYS_TO_LIST = 30;

public:
    Marketplace() : count(0) {}

    ~Marketplace() {
        for (int i = 0; i < count; i++) delete listings[i];
    }

    bool addCoupon(Coupon* c, User& adder) {
        if (c->daysLeft() < MIN_DAYS_TO_LIST) { delete c; return false; }
        if (count >= MAX_LISTINGS)            { delete c; return false; }

        for (int i = 0; i < count; i++)
            if (listings[i]->getCode() == c->getCode()) { delete c; return false; }

        listings[count] = c;
        bids[count]     = Bid();
        count++;
        adder.addCoins(100);
        saveListings();
        return true;
    }

    void cleanupExpired() {
        int w = 0;
        for (int i = 0; i < count; i++) {
            if (!listings[i]->isExpired()) {
                if (w != i) {
                    listings[w] = listings[i];
                    bids[w]     = bids[i];
                }
                w++;
            } else {
                delete listings[i];
            }
        }
        count = w;
        saveListings();
    }

    void view() const {
        if (count == 0) { cout << "Marketplace is empty.\n"; return; }
        cout << "\n=== MARKETPLACE ===\n";
        for (int i = 0; i < count; i++) {
            if (listings[i]->isExpired()) continue;
            cout << (i + 1) << ". " << listings[i]->getBrand()
                 << " — " << listings[i]->marketLabel()
                 << "  (" << listings[i]->daysLeft() << "d left)\n";
        }
    }

    bool viewDetail(int listingIndex) const {
        if (listingIndex < 1 || listingIndex > count) return false;
        int idx = listingIndex - 1;

        cout << "\n=============================\n"
             << "  COUPON DETAILS\n"
             << "=============================\n"
             << "  Brand:       " << listings[idx]->getBrand() << "\n"
             << "  Offer:       " << listings[idx]->marketLabel() << "\n"
             << "  Days left:   " << listings[idx]->daysLeft() << "\n";

        if (bids[idx].active) {
            cout << "  Highest bid: " << bids[idx].amount
                 << " coins (by " << bids[idx].bidderName << ")\n"
                 << "  You need >" << bids[idx].amount
                 << " to outbid, or 200+ for instant buy.\n";
        } else {
            cout << "  Highest bid: none yet\n"
                 << "  You need 200+ for instant buy, or bid lower to wait.\n";
        }

        cout << "=============================\n";
        return true;
    }

    int size() const { return count; }

    int placeBid(int listingIndex, User& bidder, long amount) {
        if (listingIndex < 1 || listingIndex > count) return 2;
        int idx = listingIndex - 1;

        if (!bidder.canAfford(amount)) return 2;
        if (bids[idx].active && amount <= bids[idx].amount) return 3;

        if (amount >= 200) {
            bidder.spendCoins(amount);
            bidder.addCoupon(listings[idx]);

            for (int i = idx; i < count - 1; i++) {
                listings[i] = listings[i + 1];
                bids[i]     = bids[i + 1];
            }
            count--;
            saveListings();
            return 1;
        }

        bids[idx] = Bid(bidder.getName(), amount);
        saveListings();
        return 0;
    }

    void seed(User& dummy) {
        Date d60 = Date::fromDaysFromNow(60);
        addCoupon(new FlatCoupon("SWIGGY100", "Swiggy", 299, d60, 100), dummy);
        addCoupon(new PercentCoupon("MYNTRA20", "Myntra", 999, d60, 20, 200), dummy);
        addCoupon(new BOGOCoupon("PSBOGO", "PS Store", 500, d60), dummy);
        addCoupon(new ReferralCoupon("REFER50", "Amazon", 100, d60, 50), dummy);
    }

    void saveListings() const {
        ofstream fout("listings.txt");
        for (int i = 0; i < count; i++)
            fout << listings[i]->saveLine() << '\n';
    }

    void loadListings() {
        ifstream fin("listings.txt");
        if (!fin) return;
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string type, code, brand, mc, dl, e1, e2;
            getline(ss, type, ',');
            getline(ss, code, ',');
            getline(ss, brand, ',');
            getline(ss, mc, ',');
            getline(ss, dl, ',');
            getline(ss, e1, ',');

            Date e = Date::fromDaysFromNow(stoi(dl));
            Coupon* c = nullptr;

            if (type == "FLAT")
                c = new FlatCoupon(code, brand, stod(mc), e, stod(e1));
            else if (type == "PERCENT") {
                getline(ss, e2, ',');
                c = new PercentCoupon(code, brand, stod(mc), e, stod(e1), stod(e2));
            }
            else if (type == "BOGO")
                c = new BOGOCoupon(code, brand, stod(mc), e);
            else if (type == "REFERRAL")
                c = new ReferralCoupon(code, brand, stod(mc), e, stod(e1));

            if (c && count < MAX_LISTINGS) {
                listings[count] = c;
                bids[count] = Bid();
                count++;
            }
        }
    }
};

const int MAX_USERS = 50;

class AuthManager {
    User users[MAX_USERS];
    int  userCount;
    int  currentIndex;

    static constexpr const char* USER_FILE = "users.txt";

public:
    AuthManager() : userCount(0), currentIndex(-1) {
        loadUsers();
        if (userCount == 0) {
            users[userCount++] = User("demo1", "pass1", 100);
            users[userCount++] = User("demo2", "pass2", 100);
            saveUsers();
        }
    }

    bool signUp(const string& u, const string& p) {
        if (userCount >= MAX_USERS) return false;
        for (int i = 0; i < userCount; i++)
            if (users[i].getName() == u) return false;
        users[userCount++] = User(u, p, 100);
        saveUsers();
        return true;
    }

    bool logIn(const string& u, const string& p) {
        for (int i = 0; i < userCount; i++)
            if (users[i].getName() == u && users[i].checkPassword(p)) {
                currentIndex = i;
                return true;
            }
        return false;
    }

    void logOut() { currentIndex = -1; }
    bool loggedIn() const { return currentIndex >= 0; }
    User& me() { return users[currentIndex]; }

    void saveUsers() {
        ofstream fout(USER_FILE);
        for (int i = 0; i < userCount; i++)
            fout << users[i].getName() << ','
                 << users[i].getPassword() << ','
                 << users[i].getCoins() << '\n';
    }

    void loadUsers() {
        ifstream fin(USER_FILE);
        if (!fin) return;
        string line;
        while (getline(fin, line) && userCount < MAX_USERS) {
            stringstream ss(line);
            string u, p, c;
            getline(ss, u, ',');
            getline(ss, p, ',');
            getline(ss, c, ',');
            if (u.empty()) continue;
            users[userCount++] = User(u, p, stol(c));
        }
    }
};

void signUpFlow(AuthManager& auth) {
    string u, p;
    cout << "Username: "; getline(cin, u);
    cout << "Password: "; getline(cin, p);
    if (auth.signUp(u, p)) cout << "Account created with 100 coins.\n";
    else                   cout << "Username taken.\n";
}

void logInFlow(AuthManager& auth) {
    string u, p;
    cout << "Username: "; getline(cin, u);
    cout << "Password: "; getline(cin, p);
    if (auth.logIn(u, p)) cout << "Welcome, " << u << "!\n";
    else                  cout << "Invalid credentials.\n";
}

void addCouponFlow(Marketplace& market, User& me, AuthManager& auth) {
    cout << "Type (1.Flat 2.Percent 3.BOGO 4.Referral): ";
    int t; cin >> t; cin.ignore();

    string code, brand;
    double minCart, d = 0, p = 0, cap = 0, bonus = 0;

    cout << "Code: "; getline(cin, code);
    cout << "Brand: "; getline(cin, brand);
    cout << "Min cart: "; cin >> minCart; cin.ignore();

    if (t == 1) { cout << "Discount: "; cin >> d; cin.ignore(); }
    if (t == 2) { cout << "Percent: "; cin >> p;
                  cout << "Max cap: "; cin >> cap; cin.ignore(); }
    if (t == 4) { cout << "Bonus: "; cin >> bonus; cin.ignore(); }

    int daysLeft;
    cout << "Days until expiry: "; cin >> daysLeft; cin.ignore();

    if (daysLeft < 30) {
        cout << "Coupon must have at least 30 days left to be listed.\n";
        return;
    }

    Date e = Date::fromDaysFromNow(daysLeft);

    Coupon* c = nullptr;
    if (t == 1)      c = new FlatCoupon(code, brand, minCart, e, d);
    else if (t == 2) c = new PercentCoupon(code, brand, minCart, e, p, cap);
    else if (t == 3) c = new BOGOCoupon(code, brand, minCart, e);
    else             c = new ReferralCoupon(code, brand, minCart, e, bonus);

    if (market.addCoupon(c, me))
        cout << "Listed. You earned 100 coins.\n";
    else
        cout << "Could not list (duplicate code or <30 days left).\n";

    auth.saveUsers();
}

void bidFlow(Marketplace& market, User& me, AuthManager& auth) {
    market.cleanupExpired();
    cout << "\n[Balance: " << me.getCoins() << " coins]\n";
    market.view();
    if (market.size() == 0) return;

    cout << "\nEnter coupon number to view (0 to go back): ";
    int n; cin >> n; cin.ignore();
    if (n <= 0) return;

    if (!market.viewDetail(n)) {
        cout << "Invalid number.\n";
        return;
    }

    cout << "\n1. Bid on this coupon\n"
         << "2. Back to menu\n"
         << "Choose: ";
    int choice; cin >> choice; cin.ignore();

    if (choice == 2) return;

    cout << "Your bid (you have " << me.getCoins() << " coins): ";
    long amount; cin >> amount; cin.ignore();

    if (amount >= 200) {
        cout << "\nYou are about to spend " << amount
             << " coins for an INSTANT BUY.\n"
             << "   Balance after purchase: " << (me.getCoins() - amount) << " coins.\n"
             << "   Confirm? (y/n): ";
        char c; cin >> c; cin.ignore();
        if (c != 'y' && c != 'Y') { cout << "Cancelled.\n"; return; }
    }

    int result = market.placeBid(n, me, amount);
    if (result == 1) {
        cout << "Purchased! Balance: " << me.getCoins() << " coins.\n";
        auth.saveUsers();
    }
    else if (result == 0)
        cout << "Bid of " << amount << " placed.\n";
    else if (result == 2)
        cout << "Not enough coins. You have " << me.getCoins() << ".\n";
    else
        cout << "Higher bid already exists.\n";
}

void logoutSummary(User& me) {
    cout << "\n=========================================\n"
         << "  Thanks for using Coupon Marketplace!\n"
         << "=========================================\n"
         << "  Final balance:  " << me.getCoins() << " coins\n"
         << "  Coupons owned:  " << me.getOwnedCount() << "\n"
         << "=========================================\n";
}

void userMenu(AuthManager& auth, Marketplace& market) {
    while (true) {
        clearScreen();
        User& me = auth.me();
        cout << "=========================================\n"
             << "  Welcome, " << me.getName()
             << "  |  Coins: " << me.getCoins() << "\n"
             << "=========================================\n"
             << "1. Add Coupon (+100 coins)\n"
             << "2. Browse & Bid\n"
             << "3. My Coupons\n"
             << "4. Balance\n"
             << "5. Log Out\n"
             << "Choose: ";
        int ch; cin >> ch; cin.ignore();

        if (ch == 1) addCouponFlow(market, me, auth);
        else if (ch == 2) bidFlow(market, me, auth);
        else if (ch == 3) me.viewOwned();
        else if (ch == 4) cout << "Balance: " << me.getCoins() << " coins.\n";
        else if (ch == 5) {
            logoutSummary(me);
            auth.saveUsers();
            auth.logOut();
            cout << "\nPress Enter to continue...\n";
            cin.get();
            return;
        }
        cout << "\nPress Enter to continue...\n";
        cin.get();
    }
}

int main() {
    AuthManager auth;
    Marketplace market;

    ifstream test("listings.txt");
    if (test.good()) {
        test.close();
        market.loadListings();
    } else {
        User seeder("__system__", "", 0);
        market.seed(seeder);
        market.saveListings();
    }

    while (true) {
        clearScreen();
        if (!auth.loggedIn()) {
            cout << "=============================\n"
                 << "   COUPON MARKETPLACE\n"
                 << "=============================\n"
                 << "1. Sign Up\n"
                 << "2. Log In\n"
                 << "3. Exit\n"
                 << "Choose: ";
            int ch; cin >> ch; cin.ignore();
            if (ch == 1) signUpFlow(auth);
            else if (ch == 2) logInFlow(auth);
            else if (ch == 3) {
                auth.saveUsers();
                market.saveListings();
                return 0;
            }
            cout << "\nPress Enter to continue...\n";
            cin.get();
        } else {
            userMenu(auth, market);
        }
    }
}