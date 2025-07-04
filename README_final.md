# 📚 Library Management System (C++)

A lightweight, **single‑file** Library Management System implemented in modern C++.  
It authenticates **Students**, **Faculty**, and **Librarians** and keeps all state in plain‑text files.

> **Note:** All `.txt` data files must be in the **same directory** as the compiled executable to ensure proper file I/O.

---

## 🗃️ Repository Layout

```
Library-Management-System/
├─ LMS.cpp
├─ users.txt
├─ books.txt
├─ borrowed_books.txt
├─ fines.txt
└─ (original /data folder is not used at runtime)
```

---

## 📂 File Formats

### `books.txt`

```
BookID|Title|Author|Publisher|Year|ISBN|isAvailable|isReserved
```

Example:

```
Database Systems,Elmasri,Pearson,54321,1,0
```

---

### `users.txt`

```
UserID|Password|Role
```

Example:

```
admin|libPass|Librarian
alice|password123|Student
ranu|facultyPass1|Faculty
```

---

### `borrowed_books.txt`

```
UserID|BookID|BorrowDate
```

Example:

```
ranu,22233,1747267280
```

---

### `fines.txt`

```
UserID|Amount
```

Example:

```
gracia 80
ranu 120
```

---

## ❌ Error Handling

The system handles several edge cases:

- ❗ Returning unborrowed books
- ❗ Borrowing limit exceeded
- ❗ Attempting to borrow with outstanding fines
- ❗ Book already borrowed or unavailable
- ❗ Invalid credentials or roles
- ❗ Invalid or empty inputs
- ❗ Reserving a book multiple times
- ❗ Faculty borrowing restrictions (if overdue > 60 days)

---

## 📝 Notes

- All data is stored in **flat text files** and auto-loaded on startup
- Books can be searched by **title** or **author**
- **Fines**:
  - Students: ₹10 per day overdue
  - Faculty: No fine, but restrictions apply after 60 days overdue
- Borrowing limits:
  - Students: max 3 books for 15 days
  - Faculty: max 5 books for 30 days
- Users cannot borrow more if fines are unpaid or overdue books are not returned
- Each user’s borrowing info is tracked via shared account records

---

## 🚀 Quick Start

### 1 – Clone

```bash
git clone https://github.com/someshwar034/Library-Management-System.git
cd Library-Management-System
```

### 2 – Place Data Files Next to Source

```bash
cp data/*.txt .
# (or) move them if you prefer:
# mv data/*.txt .
```

> If you skip this step the program will fail to locate its data files.

### 3 – Build

```bash
g++ -std=c++17 -O2 -Wall LMS.cpp -o lms
```

*Tested on `g++ 13` (Linux) and `clang++ 17` (macOS).  
For MSVC use:*

```powershell
cl /std:c++17 /EHsc LMS.cpp
```

### 4 – Run

```bash
./lms           # or lms.exe on Windows
```

Log in with any credentials from `users.txt` (default admin account: `admin / libPass`).

---


## 📌 Login Info (Sample)

```
Username: admin
Password: libPass
Role: Librarian
```

---

Successfully developed using core C++ OOP principles, file handling, and role-based logic.