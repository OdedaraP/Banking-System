# Targeted fixes from the first round of testing

1. **Transfer amount validation:** entering `0` or malformed currency in the client now displays a clear validation message and asks for a valid positive amount again. The server also returns a specific error if an invalid transfer request is sent directly.
2. **Account activation state:** activating an already-active account now returns `Account #N is already ACTIVE.` Deactivating an already-inactive account similarly reports `already INACTIVE`.
3. **Loan assignment display:** a pending loan now displays `assigned=UNASSIGNED`; after the manager assigns it, the field displays the employee username.
4. **Transaction history balances:** history now shows the account's current balance at the top and a `balance_after` amount for each ledger row. The transaction `amount` remains the value deposited, withdrawn, transferred, or credited by an approved loan.
5. **Joint-holder creation permissions:** creating/adding a joint holder remains administrator-only through admin option 6. Customer self-registration and employee account creation create single-owner accounts.
6. **Name modification:** admin option 3 modifies the primary holder's full name, not the login username. The menu label already reflects this. The username remains the authentication identifier.


# Write-ahead journal, recovery and verification

7. **Append-only redo journal:** added `journal.db` with durable `PREPARED` and `COMMITTED` records. The prepared record contains intended post-balances, transaction metadata, and optional loan-approval metadata, and is synced before any affected balance changes.
8. **Startup recovery:** before the server accepts clients, pending prepared records are replayed. Recovery sets balances to the intended post-values, adds a transaction history row only if its `journal_id` is absent, updates loan approval status idempotently, and appends a commit marker.
9. **Loan idempotency:** the account credit, transaction-history row and approved loan status are all part of the same journaled operation. A crash between these writes cannot credit the loan a second time on retry/recovery.
10. **Failure policy:** persistence errors after a durable prepare stop the server rather than leave a partially updated store available to clients. Restarting runs recovery before serving requests. Data files and the containing directory use checked `fdatasync`/`fsync` calls for the relevant persistence points.
11. **Automated tests:** `make test` verifies 200 concurrent threaded transfers, 50 cross-process transfers, crash injection immediately after prepare, after the first transfer account write, after both transfer-account writes, after the loan transaction row, repeated recovery/idempotency, and repair of an incomplete trailing journal record.
12. **Manual failpoints:** `BANK_FAILPOINT` supports `after_prepare`, `after_account1`, `after_account2`, `after_transaction`, and `after_loan`. Use a disposable data directory; after the injected exit, restart with `env -u BANK_FAILPOINT ./server ...` to run recovery.
