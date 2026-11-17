/*
 * HOSTEL MANAGEMENT SYSTEM
 * ------------------------
 * A console-based project in C.
 *
 * Features:
 *   - Admin login
 *   - Add / view rooms
 *   - Register / view / search / update / delete students
 *   - Allocate and vacate rooms (capacity checked)
 *   - Fee management: charge monthly rent, accept payments, list defaulters
 *   - Data saved to binary files (students.dat, rooms.dat)
 *
 * Compile:  gcc hostel_management.c -o hostel
 * Run:      ./hostel        (Windows: hostel.exe)
 * Login:    username: admin    password: admin123
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STUDENT_FILE "students.dat"
#define ROOM_FILE    "rooms.dat"
#define MAX_STUDENTS 500
#define MAX_ROOMS    100

#define ADMIN_USER "admin"
#define ADMIN_PASS "admin123"

/* ---------- Data structures ---------- */

typedef struct {
    int   id;
    char  name[50];
    char  phone[15];
    char  course[30];
    int   roomNo;       /* 0 = no room allotted */
    float feesDue;
    float feesPaid;
} Student;

typedef struct {
    int   roomNo;
    int   capacity;
    int   occupied;
    float rent;         /* monthly rent per student */
} Room;

static Student students[MAX_STUDENTS];
static Room    rooms[MAX_ROOMS];
static int     studentCount = 0;
static int     roomCount    = 0;

/* ---------- Input helpers ---------- */

static void readLine(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\n")] = '\0';
}

static int readInt(const char *prompt)
{
    char line[64];
    int value;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%d", &value) == 1)
            return value;
        printf("  Invalid number. Try again.\n");
    }
}

static float readFloat(const char *prompt)
{
    char line[64];
    float value;
    while (1) {
        readLine(prompt, line, sizeof line);
        if (sscanf(line, "%f", &value) == 1 && value >= 0)
            return value;
        printf("  Invalid amount. Try again.\n");
    }
}

static void pause_screen(void)
{
    char tmp[8];
    printf("\nPress Enter to continue...");
    fgets(tmp, sizeof tmp, stdin);
}

/* ---------- File handling ---------- */

static void loadData(void)
{
    FILE *fp = fopen(STUDENT_FILE, "rb");
    if (fp) {
        studentCount = (int)fread(students, sizeof(Student), MAX_STUDENTS, fp);
        fclose(fp);
    }
    fp = fopen(ROOM_FILE, "rb");
    if (fp) {
        roomCount = (int)fread(rooms, sizeof(Room), MAX_ROOMS, fp);
        fclose(fp);
    }
}

static void saveData(void)
{
    FILE *fp = fopen(STUDENT_FILE, "wb");
    if (fp) {
        fwrite(students, sizeof(Student), studentCount, fp);
        fclose(fp);
    } else {
        printf("Error: could not save student data!\n");
    }
    fp = fopen(ROOM_FILE, "wb");
    if (fp) {
        fwrite(rooms, sizeof(Room), roomCount, fp);
        fclose(fp);
    } else {
        printf("Error: could not save room data!\n");
    }
}

/* ---------- Lookup helpers ---------- */

static int findStudent(int id)
{
    for (int i = 0; i < studentCount; i++)
        if (students[i].id == id)
            return i;
    return -1;
}

static int findRoom(int roomNo)
{
    for (int i = 0; i < roomCount; i++)
        if (rooms[i].roomNo == roomNo)
            return i;
    return -1;
}

/* ---------- Display helpers ---------- */

static void printStudentHeader(void)
{
    printf("\n%-6s %-20s %-12s %-14s %-6s %-10s %-10s\n",
           "ID", "Name", "Phone", "Course", "Room", "Due", "Paid");
    printf("------------------------------------------------------------------------------\n");
}

static void printStudent(const Student *s)
{
    char roomStr[8];
    if (s->roomNo > 0)
        snprintf(roomStr, sizeof roomStr, "%d", s->roomNo);
    else
        strcpy(roomStr, "-");
    printf("%-6d %-20s %-12s %-14s %-6s %-10.2f %-10.2f\n",
           s->id, s->name, s->phone, s->course, roomStr, s->feesDue, s->feesPaid);
}

/* ---------- Room operations ---------- */

static void addRoom(void)
{
    if (roomCount >= MAX_ROOMS) {
        printf("Room limit reached.\n");
        return;
    }
    Room r;
    r.roomNo = readInt("Room number        : ");
    if (r.roomNo <= 0) {
        printf("Room number must be positive.\n");
        return;
    }
    if (findRoom(r.roomNo) != -1) {
        printf("Room %d already exists.\n", r.roomNo);
        return;
    }
    r.capacity = readInt("Capacity (beds)    : ");
    if (r.capacity <= 0) {
        printf("Capacity must be at least 1.\n");
        return;
    }
    r.rent     = readFloat("Monthly rent/person: ");
    r.occupied = 0;

    rooms[roomCount++] = r;
    saveData();
    printf("Room %d added successfully.\n", r.roomNo);
}

static void viewRooms(void)
{
    if (roomCount == 0) {
        printf("No rooms available.\n");
        return;
    }
    printf("\n%-8s %-10s %-10s %-10s %-12s\n",
           "Room", "Capacity", "Occupied", "Vacant", "Rent/Month");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < roomCount; i++) {
        printf("%-8d %-10d %-10d %-10d %-12.2f\n",
               rooms[i].roomNo, rooms[i].capacity, rooms[i].occupied,
               rooms[i].capacity - rooms[i].occupied, rooms[i].rent);
    }
}

static void viewVacantRooms(void)
{
    int found = 0;
    printf("\n%-8s %-10s %-12s\n", "Room", "Vacant", "Rent/Month");
    printf("--------------------------------\n");
    for (int i = 0; i < roomCount; i++) {
        if (rooms[i].occupied < rooms[i].capacity) {
            printf("%-8d %-10d %-12.2f\n", rooms[i].roomNo,
                   rooms[i].capacity - rooms[i].occupied, rooms[i].rent);
            found = 1;
        }
    }
    if (!found)
        printf("No vacant beds. Hostel is full.\n");
}

/* ---------- Student operations ---------- */

static void addStudent(void)
{
    if (studentCount >= MAX_STUDENTS) {
        printf("Student limit reached.\n");
        return;
    }
    Student s;
    memset(&s, 0, sizeof s);

    s.id = readInt("Student ID         : ");
    if (s.id <= 0) {
        printf("ID must be positive.\n");
        return;
    }
    if (findStudent(s.id) != -1) {
        printf("A student with ID %d already exists.\n", s.id);
        return;
    }
    readLine("Name               : ", s.name,   sizeof s.name);
    readLine("Phone              : ", s.phone,  sizeof s.phone);
    readLine("Course             : ", s.course, sizeof s.course);
    if (s.name[0] == '\0') {
        printf("Name cannot be empty.\n");
        return;
    }
    s.roomNo = 0;

    students[studentCount++] = s;
    saveData();
    printf("Student registered. Use 'Allocate Room' to assign a room.\n");
}

static void viewStudents(void)
{
    if (studentCount == 0) {
        printf("No students registered.\n");
        return;
    }
    printStudentHeader();
    for (int i = 0; i < studentCount; i++)
        printStudent(&students[i]);
    printf("\nTotal students: %d\n", studentCount);
}

static void searchStudent(void)
{
    printf("1. Search by ID\n2. Search by Name\n");
    int ch = readInt("Choice: ");

    if (ch == 1) {
        int id = readInt("Enter ID: ");
        int idx = findStudent(id);
        if (idx == -1) {
            printf("Student not found.\n");
            return;
        }
        printStudentHeader();
        printStudent(&students[idx]);
    } else if (ch == 2) {
        char key[50];
        readLine("Enter name (or part of it): ", key, sizeof key);
        int found = 0;
        for (int i = 0; i < studentCount; i++) {
            /* case-insensitive substring match */
            char a[50], b[50];
            strcpy(a, students[i].name);
            strcpy(b, key);
            for (char *p = a; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;
            for (char *p = b; *p; p++) if (*p >= 'A' && *p <= 'Z') *p += 32;
            if (b[0] != '\0' && strstr(a, b)) {
                if (!found) printStudentHeader();
                printStudent(&students[i]);
                found = 1;
            }
        }
        if (!found)
            printf("No matching student found.\n");
    } else {
        printf("Invalid choice.\n");
    }
}

static void updateStudent(void)
{
    int id = readInt("Enter student ID to update: ");
    int idx = findStudent(id);
    if (idx == -1) {
        printf("Student not found.\n");
        return;
    }
    char buf[50];
    printf("(Press Enter to keep the current value)\n");

    printf("Current name  : %s\n", students[idx].name);
    readLine("New name      : ", buf, sizeof buf);
    if (buf[0]) strcpy(students[idx].name, buf);

    printf("Current phone : %s\n", students[idx].phone);
    readLine("New phone     : ", buf, 15);
    if (buf[0]) strcpy(students[idx].phone, buf);

    printf("Current course: %s\n", students[idx].course);
    readLine("New course    : ", buf, 30);
    if (buf[0]) strcpy(students[idx].course, buf);

    saveData();
    printf("Student record updated.\n");
}

/* ---------- Allocation ---------- */

static void allocateRoom(void)
{
    int id = readInt("Student ID: ");
    int si = findStudent(id);
    if (si == -1) {
        printf("Student not found.\n");
        return;
    }
    if (students[si].roomNo != 0) {
        printf("%s is already in room %d. Vacate first.\n",
               students[si].name, students[si].roomNo);
        return;
    }
    viewVacantRooms();
    int rn = readInt("\nRoom number to allocate: ");
    int ri = findRoom(rn);
    if (ri == -1) {
        printf("Room not found.\n");
        return;
    }
    if (rooms[ri].occupied >= rooms[ri].capacity) {
        printf("Room %d is full.\n", rn);
        return;
    }
    rooms[ri].occupied++;
    students[si].roomNo = rn;
    students[si].feesDue += rooms[ri].rent;   /* first month's rent */
    saveData();
    printf("Room %d allocated to %s. First month's rent %.2f added to dues.\n",
           rn, students[si].name, rooms[ri].rent);
}

static void vacateRoom(void)
{
    int id = readInt("Student ID: ");
    int si = findStudent(id);
    if (si == -1) {
        printf("Student not found.\n");
        return;
    }
    if (students[si].roomNo == 0) {
        printf("%s has no room allotted.\n", students[si].name);
        return;
    }
    if (students[si].feesDue > 0)
        printf("Warning: %s still has dues of %.2f.\n",
               students[si].name, students[si].feesDue);

    int ri = findRoom(students[si].roomNo);
    if (ri != -1 && rooms[ri].occupied > 0)
        rooms[ri].occupied--;
    printf("%s vacated room %d.\n", students[si].name, students[si].roomNo);
    students[si].roomNo = 0;
    saveData();
}

static void deleteStudent(void)
{
    int id = readInt("Enter student ID to delete: ");
    int idx = findStudent(id);
    if (idx == -1) {
        printf("Student not found.\n");
        return;
    }
    if (students[idx].feesDue > 0) {
        printf("Cannot delete: student has pending dues of %.2f.\n",
               students[idx].feesDue);
        return;
    }
    if (students[idx].roomNo != 0) {
        int ri = findRoom(students[idx].roomNo);
        if (ri != -1 && rooms[ri].occupied > 0)
            rooms[ri].occupied--;
    }
    for (int i = idx; i < studentCount - 1; i++)
        students[i] = students[i + 1];
    studentCount--;
    saveData();
    printf("Student record deleted.\n");
}

/* ---------- Fee management ---------- */

static void chargeMonthlyRent(void)
{
    int count = 0;
    for (int i = 0; i < studentCount; i++) {
        if (students[i].roomNo != 0) {
            int ri = findRoom(students[i].roomNo);
            if (ri != -1) {
                students[i].feesDue += rooms[ri].rent;
                count++;
            }
        }
    }
    saveData();
    printf("Monthly rent added to dues of %d resident(s).\n", count);
}

static void payFees(void)
{
    int id = readInt("Student ID: ");
    int idx = findStudent(id);
    if (idx == -1) {
        printf("Student not found.\n");
        return;
    }
    printf("%s - current dues: %.2f\n", students[idx].name, students[idx].feesDue);
    if (students[idx].feesDue <= 0) {
        printf("No dues pending.\n");
        return;
    }
    float amt = readFloat("Amount paid: ");
    if (amt > students[idx].feesDue) {
        printf("Amount exceeds dues. Payment cancelled.\n");
        return;
    }
    students[idx].feesDue  -= amt;
    students[idx].feesPaid += amt;
    saveData();
    printf("Payment received. Remaining dues: %.2f\n", students[idx].feesDue);
}

static void viewDefaulters(void)
{
    int found = 0;
    for (int i = 0; i < studentCount; i++) {
        if (students[i].feesDue > 0) {
            if (!found) printStudentHeader();
            printStudent(&students[i]);
            found = 1;
        }
    }
    if (!found)
        printf("No fee defaulters. All dues are clear!\n");
}

/* ---------- Reports ---------- */

static void showSummary(void)
{
    int totalBeds = 0, occupied = 0;
    float totalDue = 0, totalPaid = 0;

    for (int i = 0; i < roomCount; i++) {
        totalBeds += rooms[i].capacity;
        occupied  += rooms[i].occupied;
    }
    for (int i = 0; i < studentCount; i++) {
        totalDue  += students[i].feesDue;
        totalPaid += students[i].feesPaid;
    }
    printf("\n========== HOSTEL SUMMARY ==========\n");
    printf("Total rooms        : %d\n", roomCount);
    printf("Total beds         : %d\n", totalBeds);
    printf("Occupied beds      : %d\n", occupied);
    printf("Vacant beds        : %d\n", totalBeds - occupied);
    printf("Registered students: %d\n", studentCount);
    printf("Total fees collected: %.2f\n", totalPaid);
    printf("Total fees pending  : %.2f\n", totalDue);
    printf("====================================\n");
}

/* ---------- Menus ---------- */

static int login(void)
{
    char user[30], pass[30];
    for (int attempt = 1; attempt <= 3; attempt++) {
        readLine("Username: ", user, sizeof user);
        readLine("Password: ", pass, sizeof pass);
        if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0)
            return 1;
        printf("Wrong credentials. Attempts left: %d\n\n", 3 - attempt);
    }
    return 0;
}

static void studentMenu(void)
{
    int ch;
    do {
        printf("\n--- STUDENT MANAGEMENT ---\n");
        printf("1. Register student\n");
        printf("2. View all students\n");
        printf("3. Search student\n");
        printf("4. Update student details\n");
        printf("5. Delete student\n");
        printf("0. Back\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: addStudent();    break;
            case 2: viewStudents();  break;
            case 3: searchStudent(); break;
            case 4: updateStudent(); break;
            case 5: deleteStudent(); break;
            case 0: return;
            default: printf("Invalid choice.\n");
        }
        pause_screen();
    } while (1);
}

static void roomMenu(void)
{
    int ch;
    do {
        printf("\n--- ROOM MANAGEMENT ---\n");
        printf("1. Add room\n");
        printf("2. View all rooms\n");
        printf("3. View vacant rooms\n");
        printf("4. Allocate room to student\n");
        printf("5. Vacate room\n");
        printf("0. Back\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: addRoom();         break;
            case 2: viewRooms();       break;
            case 3: viewVacantRooms(); break;
            case 4: allocateRoom();    break;
            case 5: vacateRoom();      break;
            case 0: return;
            default: printf("Invalid choice.\n");
        }
        pause_screen();
    } while (1);
}

static void feeMenu(void)
{
    int ch;
    do {
        printf("\n--- FEE MANAGEMENT ---\n");
        printf("1. Charge monthly rent to all residents\n");
        printf("2. Accept fee payment\n");
        printf("3. View fee defaulters\n");
        printf("0. Back\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: chargeMonthlyRent(); break;
            case 2: payFees();           break;
            case 3: viewDefaulters();    break;
            case 0: return;
            default: printf("Invalid choice.\n");
        }
        pause_screen();
    } while (1);
}

int main(void)
{
    printf("=====================================\n");
    printf("      HOSTEL MANAGEMENT SYSTEM       \n");
    printf("=====================================\n");

    if (!login()) {
        printf("Too many failed attempts. Exiting.\n");
        return 1;
    }
    loadData();

    int ch;
    do {
        printf("\n========== MAIN MENU ==========\n");
        printf("1. Student management\n");
        printf("2. Room management\n");
        printf("3. Fee management\n");
        printf("4. Hostel summary\n");
        printf("0. Exit\n");
        ch = readInt("Choice: ");
        switch (ch) {
            case 1: studentMenu(); break;
            case 2: roomMenu();    break;
            case 3: feeMenu();     break;
            case 4: showSummary(); pause_screen(); break;
            case 0: saveData(); printf("Data saved. Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);

    return 0;
}
