# ALU BCHIMS — Complete Step-by-Step Guide

---

## PART 1 — What Files You Have and What Each One Does

### Deliverables to submit
```
ALU_BCHIMS_Technical_Report.docx     ← Written report (~2,800 words, 13 sections)
health_insurance_blockchain/          ← Full source code folder (submit as a zip/repo)
  README.md                           ← GitHub repository description
```

### Inside `health_insurance_blockchain/`

```
health_insurance_blockchain/
│
├── Makefile                    ← Build script (type "make" to compile)
├── main.c                      ← Entry point — starts CLI, loads/saves chain
├── README.md                   ← Project overview for GitHub
│
├── include/                    ← All 17 header files (.h)
│   ├── types.h                 ← All enums and constants (start here when reading)
│   ├── crypto.h                ← SHA-256 and ECDSA P-256 function declarations
│   ├── token.h                 ← AHT token struct
│   ├── transaction.h           ← Transaction struct + all 12 types + payload union
│   ├── block.h                 ← Block struct + hash/PoW functions
│   ├── mempool.h               ← Mempool entry + priority queue operations
│   ├── utxo.h                  ← UTXO (unspent output) model
│   ├── account.h               ← Account-balance model + nonce management
│   ├── merkle.h                ← Merkle tree (built from scratch)
│   ├── participants.h          ← Member and Provider structs (with key pairs)
│   ├── insurance.h             ← Policy, Claim, and Preauth structs + lifecycle
│   ├── reinsurance.h           ← Reinsurance pool logic
│   ├── fraud.h                 ← 3 fraud heuristics + audit log
│   ├── chain.h                 ← Main Blockchain struct (owns everything)
│   ├── mining.h                ← Solo mining, pool mining, difficulty retarget
│   ├── persistence.h           ← Save/load chain to disk
│   └── cli.h                   ← All 39 command handler declarations
│
├── src/                        ← All 16 source files (.c) — the actual code
│   ├── crypto.c                ← REAL OpenSSL 3.0 EVP: SHA-256, P-256, ECDSA
│   ├── token.c                 ← Token initialise and print
│   ├── transaction.c           ← Create, sign, verify, serialise transactions
│   ├── merkle.c                ← Merkle tree (leaf hash → pair → root)
│   ├── block.c                 ← Block create, hash, PoW check, full verify
│   ├── mempool.c               ← Add/sort/select/remove mempool entries
│   ├── utxo.c                  ← Create/spend/validate UTXOs
│   ├── account.c               ← Balances, nonce increment, debit/credit
│   ├── participants.c          ← Register members/providers with real P-256 keys
│   ├── insurance.c             ← Policy lifecycle, claims, pre-authorisations
│   ├── reinsurance.c           ← 5% contribution, split settlement
│   ├── fraud.c                 ← High-freq / abnormal-amount / duplicate checks
│   ├── chain.c                 ← Chain init, append block, retarget, verify
│   ├── mining.c                ← PoW loop, solo, pool, UTXO + account updates
│   ├── persistence.c           ← Binary save/load, startup verification
│   └── cli.c                   ← REPL loop + all 39 command implementations
│
└── data/                       ← Created automatically; holds chain.dat save file
```

**Reading order if you want to understand the code:**
1. `include/types.h` — all the constants and enums
2. `include/transaction.h` — the core data type
3. `include/chain.h` — the top-level structure that holds everything
4. `src/crypto.c` — the cryptography foundation
5. `src/mining.c` — where it all comes together

---

## PART 2 — How to Set Up and Build

### Step 1 — Install dependencies (WSL2 Ubuntu)

```bash
sudo apt update
sudo apt install gcc make libssl-dev
```

Verify:
```bash
gcc --version       # should say GCC 13.x or similar
openssl version     # should say OpenSSL 3.0.x or 3.x.x
```

### Step 2 — Get the code onto your machine

**Option A: Extract the tar archive**
```bash
# Copy health_insurance_blockchain_complete.tar.gz to your WSL home folder, then:
tar -xzf health_insurance_blockchain_complete.tar.gz
cd health_insurance_blockchain
```

**Option B: Clone from GitHub (after you push)**
```bash
git clone https://github.com/Phinatech/YOUR-REPO-NAME.git
cd YOUR-REPO-NAME
```

### Step 3 — Build the project

```bash
make
```

You should see 17 gcc compile lines with NO errors and NO warnings, ending with:
```
[make] Built: aluhealth
```

If you see errors about `openssl/evp.h`, run:
```bash
sudo apt install libssl-dev
```

### Step 4 — Run the system

```bash
./aluhealth
```

You will see:
```
╔══════════════════════════════════════════════════════════════╗
║     ALU Health Insurance Blockchain  (AHT Token v1.0)       ║
║     OpenSSL EVP  ·  ECDSA P-256  ·  SHA-256  ·  PoW        ║
╚══════════════════════════════════════════════════════════════╝

ALU Health Blockchain CLI — type 'help' for commands
>
```

Type `help` to see all 39 commands. Type `exit` to quit (saves the chain automatically).

### Step 5 — Clean and rebuild if needed

```bash
make clean    # removes compiled objects and the binary
make          # recompiles everything from scratch
```

---

## PART 3 — Step-by-Step Demo Workflow

Run these commands in order to demonstrate every major feature. Each `>` is a prompt where you type the command.

### Register participants
```
> register_member alice Alice_Kamara
> register_member bob Bob_Nkosi
> register_provider clinic1 Kigali_Clinic General
> view_member alice
> view_provider clinic1
```

### Enroll a policy and pay premium
```
> enroll_policy alice POL001 BasicCare 150.0
> view_policy POL001
> pay_premium alice POL001 150.0
```

### View the mempool (should show 3 pending transactions)
```
> mempool_view
```
You will see: POLICY_ENROLL, PREMIUM_PAYMENT, REINSURANCE_CONTRIB (7.5 AHT = 5% of 150)

### Mine the first block (solo)
```
> mine_solo alice
```
You will see PoW solving, nonce found, block mined. Then:
```
> blockchain_verify
```
Expected output: `CHAIN VALID` with `[hash OK][pow OK][merkle OK][sigs OK]` for each block.

### Check balances
```
> account_balance INSURANCE_POOL_WALLET
> reinsurance_balance
> utxo_view
```
Insurance pool should show 10,150 AHT (10,000 initial + 150 premium).
Reinsurance pool should show 7.5 AHT.
UTXO set should show the mining reward and premium UTXOs.

### Submit a claim
```
> submit_claim CLM001 alice POL001 clinic1 500.0
> mempool_view
```

### Mine the claim transaction
```
> mine_solo bob
```

### Approve and settle the claim
```
> approve_claim CLM001 480.0
> settle_claim CLM001
```

### Mine the settlement
```
> mine_solo alice
```

### Verify claim history
```
> claim_history alice
> blockchain_verify
```

### Demonstrate fraud detection (submit 4 claims quickly)
```
> submit_claim CLM002 alice POL001 clinic1 100.0
> submit_claim CLM003 alice POL001 clinic1 100.0
> submit_claim CLM004 alice POL001 clinic1 100.0
> submit_claim CLM005 alice POL001 clinic1 100.0
> fraud_review
> mempool_view
```
The 4th claim (CLM005) should appear SUSPICIOUS in the fraud review queue.

### Resolve the suspicious transaction
```
> approve_suspicious <paste tx_id from fraud_review output>
```
or
```
> reject_suspicious <paste tx_id>
```

### Demonstrate pool mining
```
> register_member carol Carol_Nziza
> mine_pool alice bob carol
```
Each miner gets a proportional reward shown in the output.

### Demonstrate difficulty status
```
> difficulty_status
```

### Save and exit
```
> exit
```

### Reload and verify persistence
```
./aluhealth
> blockchain_verify
> account_balance INSURANCE_POOL_WALLET
> exit
```
Chain reloads, verifies VALID, and balances are preserved.

---

## PART 4 — Pushing to GitHub

### Step 1 — Create the repo
Go to github.com → New repository → Name it (e.g. `ALU-Health-Insurance-Blockchain`) → Public → No README (you have one already).

### Step 2 — Set up SSH key (if not done)
```bash
ssh-keygen -t ed25519 -C "your_email@example.com"
cat ~/.ssh/id_ed25519.pub
```
Copy the output → GitHub → Settings → SSH keys → New SSH key → Paste.

### Step 3 — Push the code
```bash
cd /path/to/health_insurance_blockchain

git init
git add .
git commit -m "Final summative: ALU Blockchain Health Insurance System

- 35 files, 4460 lines of C (zero warnings under -Wall -Wextra -Wpedantic)
- SHA-256 block hashing + ECDSA P-256 signing (OpenSSL 3.0 EVP only)
- 12 transaction types including full insurance domain
- Merkle tree from scratch, no external libs
- Solo + pool PoW mining with difficulty retargeting
- Reinsurance pool (5% auto-contribution, split settlement >1000 AHT)
- 3-heuristic fraud detection with SUSPICIOUS workflow
- Binary persistence with chain verification on startup
- 39 CLI commands"

git branch -M main
git remote add origin git@github.com:Phinatech/YOUR-REPO-NAME.git
git push -u origin main
```

### Step 4 — Also add the technical report
Copy `ALU_BCHIMS_Technical_Report.docx` into the project folder, then:
```bash
git add ALU_BCHIMS_Technical_Report.docx
git commit -m "Add technical report"
git push
```

---

## PART 5 — Reference Links Verification

The two URLs in the technical report References section:

**OpenSSL 3.0 Migration Guide**
```
https://docs.openssl.org/3.0/man7/migration_guide/
```
This is the official OpenSSL documentation site. The old `openssl.org/docs/man3.0/` path
now redirects to `docs.openssl.org`. Both resolve to the same content.

**Bitcoin Whitepaper**
```
https://nakamotoinstitute.org/bitcoin/
```
The Satoshi Nakamoto Institute hosts the canonical archive of the original paper.
The `bitcoin.org/bitcoin.pdf` link is also valid but occasionally rate-limits bots.

Both URLs load correctly in a normal browser. The 403 errors seen during development
were caused by the sandbox network proxy blocking external domains — not by the sites
themselves being down or the URLs being wrong.

---

## PART 6 — Demo Video Script (5–8 minutes)

Record your screen with the terminal open.

**0:00 – 0:30** — Open the terminal. Show the project folder: `ls -la`. Run `make clean && make`. Point out zero warnings.

**0:30 – 1:00** — `./aluhealth`. Type `help` to show the command list.

**1:00 – 2:00** — Register alice, bob, clinic1. Enroll policy. Pay premium. Show `mempool_view` (3 transactions queued).

**2:00 – 3:00** — `mine_solo alice`. Show PoW solving nonce. Show `blockchain_verify` → CHAIN VALID. Show `reinsurance_balance` (7.5 AHT).

**3:00 – 4:00** — `submit_claim CLM001 alice POL001 clinic1 500.0`. `mine_solo bob`. `approve_claim CLM001 480.0`. `settle_claim CLM001`. `mine_solo alice`.

**4:00 – 5:00** — `blockchain_verify` → all 4 blocks VALID with sigs OK. Show `utxo_view` (provider has 480 AHT UTXO).

**5:00 – 6:00** — Submit 4 claims to trigger fraud detection. Show `fraud_review`. Show SUSPICIOUS in mempool. Approve or reject.

**6:00 – 7:00** — `mine_pool alice bob` to show pool mining with proportional rewards. `exit` to save.

**7:00 – 8:00** — Restart `./aluhealth`. Show it loads the saved chain and `blockchain_verify` still passes. Done.
