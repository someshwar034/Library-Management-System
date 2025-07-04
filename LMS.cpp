#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <unordered_map>
using namespace std;

// ============================
//         Book Class
// ============================
class Book
{
public:
    string title, author, publisher, isbn;
    bool isAvailable;
    bool isReserved;

    Book() = default;
    Book(string t, string a, string p, string i,
         bool avail = true, bool reserved = false)
        : title(move(t)), author(move(a)), publisher(move(p)),
          isbn(move(i)), isAvailable(avail), isReserved(reserved) {}
};

// ============================
// Global Data & Persistence
// ============================
vector<Book> library; // all books in the library

struct BorrowRecord
{ // a single loan
    string username;
    string isbn;
    time_t borrowDate; // epoch seconds
};
vector<BorrowRecord> borrowedRecords; // active loans

unordered_map<string, int> outstandingFines; // username → unpaid fine (₹)

// ---------- Books -------------
void saveBooksToFile()
{
    ofstream file("books.txt");
    for (auto &b : library)
    {
        file << b.title << ',' << b.author << ',' << b.publisher << ','
             << b.isbn << ',' << (b.isAvailable ? '1' : '0') << ','
             << (b.isReserved ? '1' : '0') << '\n';
    }
}

void loadBooksFromFile()
{
    library.clear();
    ifstream file("books.txt");
    string line;
    while (getline(file, line))
    {
        stringstream ss(line);
        string title, author, publisher, isbn, availStr, resStr;
        getline(ss, title, ',');
        getline(ss, author, ',');
        getline(ss, publisher, ',');
        getline(ss, isbn, ',');
        getline(ss, availStr, ',');
        getline(ss, resStr, ',');
        bool avail = (availStr == "1");
        bool reserved = (resStr == "1");
        library.emplace_back(title, author, publisher, isbn, avail, reserved);
    }
}

// ---------- Borrowed records -------------
void saveBorrowedToFile()
{
    ofstream f("borrowed_books.txt");
    for (auto &rec : borrowedRecords)
        f << rec.username << ',' << rec.isbn << ',' << rec.borrowDate << '\n';
}

void loadBorrowedFromFile()
{
    borrowedRecords.clear();
    ifstream f("borrowed_books.txt");
    string line;
    while (getline(f, line))
    {
        stringstream ss(line);
        string user, isbn, tStr;
        getline(ss, user, ',');
        getline(ss, isbn, ',');
        getline(ss, tStr, ',');
        if (user.empty() || isbn.empty() || tStr.empty())
            continue;
        time_t t = static_cast<time_t>(stoll(tStr));
        borrowedRecords.push_back({user, isbn, t});
    }
}

// ---------- Fines -------------
void saveFinesToFile()
{
    ofstream f("fines.txt");
    for (auto &kv : outstandingFines)
        if (kv.second > 0)
            f << kv.first << ' ' << kv.second << '\n';
}

void loadFinesFromFile()
{
    outstandingFines.clear();
    ifstream f("fines.txt");
    string user;
    int amt;
    while (f >> user >> amt)
        outstandingFines[user] = amt;
}

// ---------- Helpers -------------
int countBorrowed(const string &user)
{
    int c = 0;
    for (auto &r : borrowedRecords)
        if (r.username == user)
            ++c;
    return c;
}

bool hasOverdueFaculty(const string &user)
{
    time_t now = time(nullptr);
    for (auto &r : borrowedRecords)
        if (r.username == user && (now - r.borrowDate) / (60 * 60 * 24) > 60)
            return true;
    return false;
}

int calcDynamicOverdueFine(const string &user)
{
    time_t now = time(nullptr);
    int fine = 0;
    for (auto &r : borrowedRecords)
    {
        if (r.username == user)
        {
            int days = static_cast<int>((now - r.borrowDate) / (60 * 60 * 24));
            int overdue = days - 15; // 15‑day window for students
            if (overdue > 0)
                fine += overdue * 10;
        }
    }
    return fine;
}

// ============================
//          User Base
// ============================
class User
{
protected:
    string username, password, role;

public:
    User(string u, string p, string r) : username(move(u)), password(move(p)), role(move(r)) {}
    virtual ~User() = default;

    virtual void borrowBook(Book &book) = 0;
    virtual void returnBook(Book &book) = 0;

    virtual void addBook() { cout << "\nPermission denied!\n"; }
    virtual void removeBook() { cout << "\nPermission denied!\n"; }
    virtual void addUser() { cout << "\nPermission denied!\n"; }
    virtual void removeUser() { cout << "\nPermission denied!\n"; }
    virtual void viewBorrowedBooks() { cout << "\nBorrow view not available.\n"; }
    virtual void reserveBook(Book &) { cout << "\nPermission denied!\n"; }

    virtual void payFine() { cout << "\nYou cannot pay directly; Pay Fine to the Librarian.\n"; }
    virtual int getFineAmount() { return 0; }

    string getUsername() const { return username; }
    string getRole() const { return role; }
};

// ============================
//          Student
// ============================
class Student : public User
{
    int maxBooks = 3;

public:
    Student(string u, string p) : User(move(u), move(p), "Student") {}

    // ----- view current loans -----
    void viewBorrowedBooks() override
    {
        cout << "\nYour Borrowed Books:\n";
        for (auto &r : borrowedRecords)
            if (r.username == username)
            {
                cout << "\nISBN: " << r.isbn << ", Borrowed on: " << ctime(&r.borrowDate);
            }
    }

    // ----- borrow -----
    void borrowBook(Book &book) override
    {
        if (getFineAmount() > 0)
        {
            cout << "\nCannot borrow until all fines are cleared and overdue books are returned!\n";
            return;
        }
        if (countBorrowed(username) >= maxBooks)
        {
            cout << "\nBorrow limit (3) reached!\n";
            return;
        }
        if (!book.isAvailable)
        {
            cout << "\nBook is not available!\n";
            return;
        }

        borrowedRecords.push_back({username, book.isbn, time(nullptr)});
        book.isAvailable = false;
        book.isReserved = false;
        saveBorrowedToFile();
        saveBooksToFile();
        cout << "\nBorrowed: " << book.title << '\n';
    }

    // ----- return -----
    void returnBook(Book &book) override
    {
        for (auto it = borrowedRecords.begin(); it != borrowedRecords.end(); ++it)
        {
            if (it->username == username && it->isbn == book.isbn)
            {
                int days = static_cast<int>((time(nullptr) - it->borrowDate) / (60 * 60 * 24));
                int overdue = days - 15;
                if (overdue > 0)
                {
                    int fine = overdue * 10;
                    outstandingFines[username] += fine; // accumulate permanent fine
                    saveFinesToFile();
                    cout << "\nOverdue by " << overdue << " days. Fine added: Rs." << fine << '\n';
                }
                borrowedRecords.erase(it);
                book.isAvailable = true;
                book.isReserved = false;
                saveBorrowedToFile();
                saveBooksToFile();
                cout << "Returned: " << book.title << '\n';
                return;
            }
        }
        cout << "\nYou haven't borrowed this book!\n";
    }

    // ----- reserve -----
    void reserveBook(Book &book) override
    {
        if (book.isAvailable)
        {
            cout << "\nBook is available — No need to reserve.\n";
        }
        else if (book.isReserved)
        {
            cout << "\nBook already reserved by someone else.\n";
        }
        else
        {
            book.isReserved = true;
            saveBooksToFile();
            cout << "\nBook reserved successfully!\n";
        }
    }

    // ----- fines -----
    int getFineAmount() override
    {
        return outstandingFines[username] + calcDynamicOverdueFine(username);
    }
};

// ============================
//          Faculty
// ============================
class Faculty : public User
{
    int maxBooks = 5;

public:
    Faculty(string u, string p) : User(move(u), move(p), "Faculty") {}

    void viewBorrowedBooks() override
    {
        cout << "\nYour Borrowed Books:\n";
        for (auto &r : borrowedRecords)
            if (r.username == username)
                cout << "ISBN: " << r.isbn << ", Borrowed on: " << ctime(&r.borrowDate);
    }

    void borrowBook(Book &book) override
    {
        if (hasOverdueFaculty(username))
        {
            cout << "\nOverdue book (>60 days) detected—return it first!\n";
            return;
        }
        if (countBorrowed(username) >= maxBooks)
        {
            cout << "\nBorrow limit (5) reached!\n";
            return;
        }
        if (!book.isAvailable)
        {
            cout << "\nBook is not available!\n";
            return;
        }

        borrowedRecords.push_back({username, book.isbn, time(nullptr)});
        book.isAvailable = false;
        book.isReserved = false;
        saveBorrowedToFile();
        saveBooksToFile();
        cout << "Borrowed: " << book.title << '\n';
    }

    void returnBook(Book &book) override
    {
        for (auto it = borrowedRecords.begin(); it != borrowedRecords.end(); ++it)
        {
            if (it->username == username && it->isbn == book.isbn)
            {
                borrowedRecords.erase(it);
                book.isAvailable = true;
                book.isReserved = false;
                saveBorrowedToFile();
                saveBooksToFile();
                cout << "Returned: " << book.title << '\n';
                return;
            }
        }
        cout << "\nYou haven't borrowed this book!\n";
    }

    void reserveBook(Book &book) override
    {
        if (book.isAvailable)
        {
            cout << "\nBook is available—no need to reserve.\n";
        }
        else if (book.isReserved)
        {
            cout << "\nBook already reserved.\n";
        }
        else
        {
            book.isReserved = true;
            saveBooksToFile();
            cout << "\nBook reserved successfully!\n";
        }
    }
};

// ============================
//          Librarian
// ============================
class Librarian : public User
{
public:
    Librarian(string u, string p) : User(move(u), move(p), "Librarian") {}

    void borrowBook(Book &) override { cout << "Librarians cannot borrow books!\n"; }
    void returnBook(Book &) override {}
    //-------Fine Management----
    void payFine() override
    {
        string user;
        cout << "Enter the username of Student to pay Fine: ";
        cin >> user;
        if (outstandingFines.find(user) == outstandingFines.end())
        {
            cout << "\nUsername not found!!!\n";
        }
        else
        {
            int &amt = outstandingFines[user];
            if (amt == 0)
            {
                cout << "\nNo outstanding fine to pay. Fines for your currently borrowed Books will be added after you return the book\n";
            }
            else
            {
                cout << "\nThe fine for returned books of Student: " << user << " was: " << amt << " \nFine paid successfully!\n";
                amt = 0;
                saveFinesToFile();
            }
        }
    }
    // ----- book management -----
    void addBook() override
    {
        string title, author, publisher, isbn;
        cout << "Enter title: ";
        cin.ignore();
        getline(cin, title);
        cout << "Enter author: ";
        getline(cin, author);
        cout << "Enter publisher: ";
        getline(cin, publisher);
        cout << "Enter ISBN: ";
        getline(cin, isbn);

        library.emplace_back(title, author, publisher, isbn, true, false);
        saveBooksToFile();
        cout << "Book added successfully!\n";
    }

    void removeBook() override
    {
        string isbn;
        cout << "Enter ISBN to remove: ";
        cin >> isbn;
        for (auto it = library.begin(); it != library.end(); ++it)
        {
            if (it->isbn == isbn)
            {
                cout << "Book removed: " << it->title << '\n';
                library.erase(it);
                saveBooksToFile();
                return;
            }
        }
        cout << "Book not found!\n";
    }

    // ----- user management -----
    void addUser() override
    {
        string uname, pwd, role;
        cout << "Enter new username: ";
        cin >> uname;

        // check duplicate
        ifstream infile("users.txt");
        string u, p, r;
        bool exists = false;
        while (infile >> u >> p >> r)
            if (u == uname)
            {
                exists = true;
                break;
            }
        if (exists)
        {
            cout << "Username already exists!\n";
            return;
        }

        cout << "Enter password: ";
        cin >> pwd;
        cout << "Enter role (Student/Faculty/Librarian): ";
        cin >> role;

        ofstream file("users.txt", ios::app);
        if (file)
        {
            file << uname << ' ' << pwd << ' ' << role << '\n';
            cout << "User added.\n";
        }
        else
        {
            cout << "Error opening users.txt\n";
        }
    }

    void removeUser() override
    {
        string uname;
        cout << "Enter username to remove: ";
        cin >> uname;

        ifstream in("users.txt");
        ofstream tmp("temp.txt");
        string u, p, r;
        bool found = false;
        while (in >> u >> p >> r)
        {
            if (u == uname)
            {
                found = true;
                continue;
            }
            tmp << u << ' ' << p << ' ' << r << '\n';
        }
        in.close();
        tmp.close();
        remove("users.txt");
        rename("temp.txt", "users.txt");
        if (found)
            cout << "User removed.\n";
        else
            cout << "User not found!\n";
    }
};

// ============================
//         Authentication
// ============================
User *authenticateUser(const string &username, const string &password)
{
    ifstream file("users.txt");
    string u, p, r;
    while (file >> u >> p >> r)
    {
        if (u == username && p == password)
        {
            if (r == "Student")
                return new Student(u, p);
            if (r == "Faculty")
                return new Faculty(u, p);
            if (r == "Librarian")
                return new Librarian(u, p);
        }
    }
    return nullptr;
}

// ============================
//              Main
// ============================
int main()
{
    loadBooksFromFile();
    loadBorrowedFromFile();
    loadFinesFromFile();

    string username, password;
    cout << "Username: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    User *user = authenticateUser(username, password);
    if (!user)
    {
        cout << "Invalid credentials.\n";
        return 0;
    }

    cout << endl;
    cout << "<---------------------------xxxxxx--------------------------->" << endl;

    cout << "\nWelcome " << user->getUsername() << "! Role: " << user->getRole() << "\n";

    cout << endl
         << "<---------------------------xxxxxx--------------------------->" << endl;

    int choice;
    do
    {
        cout << endl
             << "Here are your functions choose any one from them" << endl;
        cout << "\n1. View Available Books\n2. View Borrowed Books\n3. Borrow Book \n4. Return Book\n5. Reserve Book \n"
             << "6. View Fine\n7. Add Book\n8. Remove Book\n9. Add User\n10. Remove User\n"
             << "11. Pay Fine\n12. Exit\n";
        cout << "\nEnter choice: ";
        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(1024, '\n');
            continue;
        }

        if (choice == 1)
        {
            cout << "\nAvailable Books:\n";
            for (auto &b : library)
                if (b.isAvailable)
                    cout << "Title: " << b.title << ", Author: " << b.author
                         << ", ISBN: " << b.isbn << '\n';
        }
        else if (choice == 2)
        {
            user->viewBorrowedBooks();
        }
        else if (choice >= 3 && choice <= 5)
        {
            cout << "\nEnter ISBN: ";
            string isbn;
            cin >> isbn;
            bool found = false;
            for (auto &b : library)
            {
                if (b.isbn == isbn)
                {
                    found = true;
                    if (choice == 3)
                        user->borrowBook(b);
                    else if (choice == 4)
                        user->returnBook(b);
                    else if (choice == 5)
                        user->reserveBook(b);
                    break;
                }
            }
            if (!found)
                cout << "Book not found!\n";
        }
        else if (choice == 6)
        {
            cout << "Outstanding Fine: Rs. " << user->getFineAmount() << '\n';
        }
        else if (choice == 7)
        {
            user->addBook();
        }
        else if (choice == 8)
        {
            user->removeBook();
        }
        else if (choice == 9)
        {
            user->addUser();
        }
        else if (choice == 10)
        {
            user->removeUser();
        }
        else if (choice == 11)
        {
            user->payFine();
        }
        cout << endl
             << "<---------------------------xxxxxx--------------------------->" << endl;
    } while (choice != 12);

    delete user;
    saveBorrowedToFile();
    saveFinesToFile();
    return 0;
}
