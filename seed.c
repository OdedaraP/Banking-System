/*
 * seed.c  —  Demo data seeder for the Online Banking OS Mini Project
 *
 * Compile & run (after building the main project):
 *   make seed
 *   ./seed [data_dir]          # default data_dir = ./data
 *
 * WARNING: Run this ONLY on a fresh / empty data directory.
 *          It will fail gracefully if data already exists.
 *
 * What it creates:
 *   Staff  : admin, manager, employee (created automatically by store_init)
 *   Customers: 10 single-owner accounts with realistic Indian names & balances
 *   Joint account: account shared between Priya and Rahul
 *   Transactions : deposits, withdrawals, and transfers between accounts
 *   Loans  : PENDING, ASSIGNED, APPROVED, REJECTED examples
 *   Feedback: 8 customer messages
 */

#define _POSIX_C_SOURCE 200809L
#include "bank.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ helpers */

static void die(const char *msg, const char *err) {
    fprintf(stderr, "SEED ERROR: %s: %s\n", msg, err);
    exit(1);
}

static void ok(const char *msg) {
    printf("  [OK] %s\n", msg);
}

static int do_deposit(int account_id, long long paise, const char *actor,
                      char *err, size_t errcap) {
    return store_money_action(0, account_id, "DEPOSIT", paise, actor, err, errcap);
}

static int do_withdraw(int account_id, long long paise, const char *actor,
                       char *err, size_t errcap) {
    return store_money_action(account_id, 0, "WITHDRAW", paise, actor, err, errcap);
}

static int do_transfer(int from, int to, long long paise, const char *actor,
                       char *err, size_t errcap) {
    return store_money_action(from, to, "TRANSFER", paise, actor, err, errcap);
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv) {
    const char *data_dir = (argc >= 2) ? argv[1] : "./data";

    printf("\n=== Online Banking Demo Data Seeder ===\n");
    printf("Data directory: %s\n\n", data_dir);

    char err[512];

    /* ---- 1. Initialize store (creates default staff on first run) ---- */
    printf("[1/7] Initializing store...\n");
    if (store_init(data_dir, err, sizeof(err)) != 0)
        die("store_init", err);
    ok("Store initialized. Default staff: admin/Admin@123, manager/Manager@123, employee/Employee@123");

    /* ---- 2. Create 10 customer accounts ---- */
    printf("\n[2/7] Creating customer accounts...\n");

    struct { const char *user; const char *pass; const char *name; } customers[] = {
        { "arjun_sharma",   "Arjun@1234",   "Arjun Sharma"       },
        { "priya_patel",    "Priya@1234",   "Priya Patel"        },
        { "rahul_verma",    "Rahul@1234",   "Rahul Verma"        },
        { "kavitha_nair",   "Kavitha@1234", "Kavitha Nair"       },
        { "suresh_kumar",   "Suresh@1234",  "Suresh Kumar"       },
        { "meera_iyer",     "Meera@1234",   "Meera Iyer"         },
        { "vikram_singh",   "Vikram@1234",  "Vikram Singh"       },
        { "ananya_reddy",   "Ananya@1234",  "Ananya Reddy"       },
        { "deepak_joshi",   "Deepak@1234",  "Deepak Joshi"       },
        { "lakshmi_menon",  "Lakshmi@1234", "Lakshmi Menon"      },
    };

    int account_ids[10];
    int n_customers = (int)(sizeof(customers) / sizeof(customers[0]));

    for (int i = 0; i < n_customers; i++) {
        User created;
        memset(&created, 0, sizeof(created));
        if (store_register_customer(customers[i].user, customers[i].pass,
                                    customers[i].name, 0, 0,
                                    &created, err, sizeof(err)) != 0) {
            fprintf(stderr, "  [SKIP] %s: %s\n", customers[i].user, err);
            User existing;
            if (store_find_user(customers[i].user, &existing) == 0)
                account_ids[i] = existing.account_id;
            else
                account_ids[i] = -1;
        } else {
            account_ids[i] = created.account_id;
            char msg[128];
            snprintf(msg, sizeof(msg), "Account #%d -> %s (%s)",
                     created.account_id, customers[i].user, customers[i].name);
            ok(msg);
        }
    }

    /* ---- 3. Add a joint account holder ---- */
    printf("\n[3/7] Adding joint account holder...\n");
    if (account_ids[1] > 0) {
        User joint;
        memset(&joint, 0, sizeof(joint));
        if (store_register_customer("rahul_joint", "Rahul@5678", "Rahul Verma (Joint)",
                                    1, account_ids[1], &joint, err, sizeof(err)) != 0) {
            fprintf(stderr, "  [SKIP] rahul_joint: %s\n", err);
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "rahul_joint added to account #%d (Priya's account)", account_ids[1]);
            ok(msg);
        }
    }

    /* ---- 4. Deposits, withdrawals, transfers ---- */
    printf("\n[4/7] Seeding transactions...\n");

    /* Initial deposits in paise (100 paise = Rs. 1) */
    long long initial_deposits[] = {
        500000000LL,  /* arjun:    Rs. 5,00,000 */
        250000000LL,  /* priya:    Rs. 2,50,000 */
        180000000LL,  /* rahul:    Rs. 1,80,000 */
        320000000LL,  /* kavitha:  Rs. 3,20,000 */
         75000000LL,  /* suresh:      Rs. 75,000 */
        400000000LL,  /* meera:    Rs. 4,00,000 */
        120000000LL,  /* vikram:   Rs. 1,20,000 */
         55000000LL,  /* ananya:      Rs. 55,000 */
        210000000LL,  /* deepak:   Rs. 2,10,000 */
        630000000LL,  /* lakshmi:  Rs. 6,30,000 */
    };

    for (int i = 0; i < n_customers; i++) {
        if (account_ids[i] <= 0) continue;
        if (do_deposit(account_ids[i], initial_deposits[i], "seed", err, sizeof(err)) != 0)
            fprintf(stderr, "  [WARN] deposit %s: %s\n", customers[i].user, err);
        else {
            char money[32]; bank_format_money(initial_deposits[i], money, sizeof(money));
            char msg[128];
            snprintf(msg, sizeof(msg), "Deposited %s to #%d (%s)",
                     money, account_ids[i], customers[i].user);
            ok(msg);
        }
    }

    /* Withdrawals */
    struct { int idx; long long paise; const char *desc; } withdrawals[] = {
        { 0, 10000000LL, "arjun ATM withdrawal Rs.1,00,000"  },
        { 4,  2000000LL, "suresh ATM withdrawal Rs.20,000"   },
        { 6,  5000000LL, "vikram ATM withdrawal Rs.50,000"   },
    };
    for (int i = 0; i < (int)(sizeof(withdrawals)/sizeof(withdrawals[0])); i++) {
        int idx = withdrawals[i].idx;
        if (account_ids[idx] <= 0) continue;
        if (do_withdraw(account_ids[idx], withdrawals[i].paise,
                        customers[idx].user, err, sizeof(err)) != 0)
            fprintf(stderr, "  [WARN] withdraw: %s\n", err);
        else
            ok(withdrawals[i].desc);
    }

    /* Transfers */
    struct { int from; int to; long long paise; const char *desc; } transfers[] = {
        { 0, 2, 5000000LL,  "arjun -> rahul Rs.50,000"       },
        { 3, 5, 2000000LL,  "kavitha -> meera Rs.20,000"     },
        { 9, 0, 10000000LL, "lakshmi -> arjun Rs.1,00,000"   },
        { 5, 1,  500000LL,  "meera -> priya Rs.5,000"        },
        { 2, 7, 1000000LL,  "rahul -> ananya Rs.10,000"      },
    };
    for (int i = 0; i < (int)(sizeof(transfers)/sizeof(transfers[0])); i++) {
        int f = transfers[i].from, t = transfers[i].to;
        if (account_ids[f] <= 0 || account_ids[t] <= 0) continue;
        if (do_transfer(account_ids[f], account_ids[t], transfers[i].paise,
                        customers[f].user, err, sizeof(err)) != 0)
            fprintf(stderr, "  [WARN] transfer: %s: %s\n", transfers[i].desc, err);
        else
            ok(transfers[i].desc);
    }

    /* ---- 5. Loan applications ---- */
    printf("\n[5/7] Creating loan applications...\n");

    struct {
        int cust_idx;
        long long paise;
        int assign;   /* 1 = assign to employee */
        int approve;  /* 1 = approve, -1 = reject, 0 = leave as assigned */
        const char *desc;
    } loans[] = {
        { 0, 100000000LL, 1,  1, "arjun:  Rs.10,00,000 HOME LOAN    -> APPROVED"  },
        { 2,  50000000LL, 1,  0, "rahul:  Rs. 5,00,000 CAR LOAN     -> ASSIGNED"  },
        { 4,  25000000LL, 1, -1, "suresh: Rs. 2,50,000 PERSONAL     -> REJECTED"  },
        { 6,  75000000LL, 0,  0, "vikram: Rs. 7,50,000 BUSINESS     -> PENDING"   },
        { 8,  10000000LL, 1,  1, "deepak: Rs. 1,00,000 EDUCATION    -> APPROVED"  },
    };

    for (int i = 0; i < (int)(sizeof(loans)/sizeof(loans[0])); i++) {
        int idx = loans[i].cust_idx;
        if (account_ids[idx] <= 0) continue;

        int loan_id = 0;
        if (store_create_loan(account_ids[idx], customers[idx].user,
                              loans[i].paise, &loan_id, err, sizeof(err)) != 0) {
            fprintf(stderr, "  [WARN] create loan for %s: %s\n", customers[idx].user, err);
            continue;
        }

        if (loans[i].assign) {
            if (store_assign_loan(loan_id, "employee", "manager", err, sizeof(err)) != 0) {
                fprintf(stderr, "  [WARN] assign loan #%d: %s\n", loan_id, err);
                ok(loans[i].desc);
                continue;
            }
            if (loans[i].approve == 1) {
                if (store_process_loan(loan_id, "employee", 1, err, sizeof(err)) != 0)
                    fprintf(stderr, "  [WARN] approve loan #%d: %s\n", loan_id, err);
            } else if (loans[i].approve == -1) {
                if (store_process_loan(loan_id, "employee", 0, err, sizeof(err)) != 0)
                    fprintf(stderr, "  [WARN] reject loan #%d: %s\n", loan_id, err);
            }
        }
        char msg[200];
        snprintf(msg, sizeof(msg), "Loan #%d - %s", loan_id, loans[i].desc);
        ok(msg);
    }

    /* ---- 6. Customer feedback ---- */
    printf("\n[6/7] Adding feedback...\n");

    struct { int idx; const char *msg; } feedbacks[] = {
        { 0, "Excellent service! The mobile banking interface is very user-friendly." },
        { 1, "The loan approval process was faster than expected. Great experience!" },
        { 3, "Transfer was processed instantly. Happy with the service." },
        { 5, "I had trouble logging in from a second device but support helped quickly." },
        { 6, "Would love to see a mobile app version of this system." },
        { 8, "Education loan process is seamless. Highly recommended." },
        { 9, "Very reliable. My salary deposits always appear on time." },
        { 2, "Please add email notifications for transactions." },
    };

    for (int i = 0; i < (int)(sizeof(feedbacks)/sizeof(feedbacks[0])); i++) {
        int idx = feedbacks[i].idx;
        if (store_add_feedback(customers[idx].user, feedbacks[i].msg, err, sizeof(err)) != 0)
            fprintf(stderr, "  [WARN] feedback from %s: %s\n", customers[idx].user, err);
        else {
            char msg[100];
            snprintf(msg, sizeof(msg), "Feedback from %s", customers[idx].user);
            ok(msg);
        }
    }

    /* ---- 7. Summary ---- */
    printf("\n[7/7] Seeding complete!\n");
    printf("\n========================================\n");
    printf("  DEMO CREDENTIALS\n");
    printf("========================================\n");
    printf("  ADMIN    : admin         / Admin@123\n");
    printf("  MANAGER  : manager       / Manager@123\n");
    printf("  EMPLOYEE : employee      / Employee@123\n");
    printf("  ----------------------------------------\n");
    printf("  CUSTOMER : arjun_sharma  / Arjun@1234\n");
    printf("  CUSTOMER : priya_patel   / Priya@1234\n");
    printf("  CUSTOMER : rahul_verma   / Rahul@1234\n");
    printf("  CUSTOMER : kavitha_nair  / Kavitha@1234\n");
    printf("  CUSTOMER : suresh_kumar  / Suresh@1234\n");
    printf("  CUSTOMER : meera_iyer    / Meera@1234\n");
    printf("  CUSTOMER : vikram_singh  / Vikram@1234\n");
    printf("  CUSTOMER : ananya_reddy  / Ananya@1234\n");
    printf("  CUSTOMER : deepak_joshi  / Deepak@1234\n");
    printf("  CUSTOMER : lakshmi_menon / Lakshmi@1234\n");
    printf("  JOINT    : rahul_joint   / Rahul@5678  (shares Priya account)\n");
    printf("========================================\n\n");
    printf("Now start the server:  ./server\n");
    printf("Then run the client:   ./client\n\n");

    return 0;
}
