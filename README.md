
# ALU Blockchain-Based Health Insurance Management System

**Course:** Introduction to Blockchain — Final Summative  
**Language:** C (C11) | **Compiler:** GCC 13.3.0 | **Crypto:** OpenSSL 3.0 EVP  
**Environment:** WSL2 Ubuntu 24.04

---

## Overview

A fully functional blockchain-based health insurance system for ALU students and staff, implemented from scratch in C. The system uses real cryptographic primitives (SHA-256, ECDSA P-256) to manage policies, premiums, claims, and settlements on an immutable chain of blocks.

```
╔══════════════════════════════════════════════════════════════╗
║     ALU Health Insurance Blockchain  (AHT Token v1.0)       ║
║     OpenSSL EVP  ·  ECDSA P-256  ·  SHA-256  ·  PoW        ║
╚══════════════════════════════════════════════════════════════╝
```

---

## Features

| Area | What's implemented |
|---|---|
| Cryptography | SHA-256 block hashing, ECDSA P-256 signing/verification (OpenSSL 3.0 EVP only) |
| Ledger models | UTXO model + Account-Balance model, both running in parallel |
| Merkle tree | Built from scratch, no external libraries |
| Mining | Proof-of-Work, solo mining, pool mining with proportional reward distribution |
| Difficulty | Auto-retarget every 10 blocks (target: 30–90 s per block) |
| Token | AHT (ALU Health Token) — minted as mining rewards |
| Transactions | 12 types: POLICY_ENROLL, PREMIUM_PAYMENT, REINSURANCE_CONTRIB, SERVICE_REQUEST, PREAUTH_REQUEST, PREAUTH_APPROVE, CLAIM_SUBMIT, CLAIM_APPROVE, CLAIM_REJECT, CLAIM_SETTLE, TOKEN_TRANSFER, MINING_REWARD |
| Policy lifecycle | ACTIVE → EXPIRED (auto-checked on claim) → RENEWED |
| Reinsurance | 5% auto-contribution per premium; split settlement above 1,000 AHT |
| Fraud detection | 3 heuristics: high-frequency claims, abnormal amount, duplicate tx ID |
| Replay protection | sender_nonce validated against account nonce at mining time |
| Persistence | Binary save/load with full chain verification on startup |
| CLI | 39 commands across 8 categories |

---

## Prerequisites

```bash
# Ubuntu / WSL2
sudo apt update
sudo apt install gcc make libssl-dev

# Verify
gcc --version        # needs ≥ 11
openssl version      # needs 3.x
```

---

## Build

```bash
git clone git@github.com:Phinatech/Blockchain_Final_Summatives_Health_insurance.git
cd Blockchain_Final_Summatives_Health_insurance
make
```

Zero warnings under `-Wall -Wextra -Wpedantic -std=c11`. The binary is named `aluhealth`.

```bash
./aluhealth          # start the CLI REPL
```

---

## Quick Start — Full Workflow

```
> register_member alice Alice_Kamara
> register_member bob Bob_Nkosi
> register_provider clinic1 Kigali_Clinic General
> enroll_policy alice POL001 BasicCare 150.0
> pay_premium alice POL001 150.0
> mempool_view
> mine_solo alice
> blockchain_verify
> submit_claim CLM001 alice POL001 clinic1 500.0
> mine_solo bob
> approve_claim CLM001 480.0
> settle_claim CLM001
> mine_solo alice
> blockchain_verify
> reinsurance_balance
> utxo_view
> exit
```

The chain is saved automatically on `exit`. Restart to reload:

```bash
./aluhealth
> blockchain_verify    # loaded chain verifies clean
```

---

## CLI Reference

### Membership
```
register_member <id> <name>
view_member <id>
register_provider <id> <name> <specialty>
view_provider <id>
wallet_balance <member_id>
```

### Policy
```
enroll_policy <member_id> <policy_id> <plan> <premium>
view_policy <policy_id>
renew_policy <policy_id>
policy_status <policy_id>
```

### Insurance Operations
```
pay_premium <member_id> <policy_id> <amount>
service_request <member_id> <provider_id> <type>
preauth_request <preauth_id> <member_id> <provider_id> <type> <est_cost>
preauth_approve <preauth_id> <approved_amt>
submit_claim <claim_id> <member_id> <policy_id> <provider_id> <amount>
approve_claim <claim_id> <approved_amt>
reject_claim <claim_id> <reason>
settle_claim <claim_id>
reinsurance_balance
```

### Token
```
token_transfer <from_address> <to_address> <amount>
token_balance <address>
```

### Blockchain
```
mempool_view
mine_solo <miner_id>
mine_pool <miner1_id> [miner2_id] ...
blockchain_view
blockchain_verify
chain_save [path]
difficulty_status
```

### UTXO & Accounts
```
utxo_view [address]
utxo_validate <utxo_id> <address>
account_balance <address>
account_transfer <from> <to> <amount>
account_nonce <address>
```

### Fraud & Audit
```
fraud_review
approve_suspicious <tx_id>
reject_suspicious <tx_id>
transaction_history
provider_history <provider_id>
premium_history <member_id>
claim_history <member_id>
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   CLI Layer (cli.c)                         │
│              39 commands, REPL dispatch table               │
└───────────┬─────────────────┬───────────────────────────────┘
            ▼                 ▼                 ▼
┌───────────────────┐ ┌───────────────┐ ┌──────────────────┐
│  Insurance Domain │ │ Mining/Mempool│ │ Token/Reinsurance │
│ policy · claim    │ │ PoW · pool    │ │ AHT · 5% contrib  │
│ preauth · fraud   │ │ retarget      │ │ split settlement  │
└───────────┬───────┘ └───────┬───────┘ └────────┬─────────┘
            ▼                 ▼                   ▼
┌───────────────────────────────────────────────────────────┐
│                  Core Blockchain Layer                     │
│  transaction.c  block.c  chain.c  mempool.c  merkle.c     │
│  account.c      utxo.c   mining.c                         │
└─────────────────────────────────┬─────────────────────────┘
                                  ▼
┌───────────────────────────────────────────────────────────┐
│                 Infrastructure Layer                       │
│         crypto.c (OpenSSL EVP)   persistence.c            │
└───────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
.
├── Makefile
├── main.c
├── include/          # 17 header files
│   ├── types.h       # all enums and constants
│   ├── crypto.h      # SHA-256, ECDSA P-256
│   ├── transaction.h # 12 tx types + payload union
│   ├── block.h       # Block struct + PoW
│   ├── chain.h       # Blockchain top-level struct
│   ├── mempool.h     # priority queue
│   ├── mining.h      # solo + pool + retarget
│   ├── insurance.h   # Policy, Claim, Preauth
│   ├── fraud.h       # 3 heuristics + audit log
│   └── ...
└── src/              # 16 source files
    ├── crypto.c      # real OpenSSL 3.0 EVP implementation
    ├── merkle.c      # Merkle tree from scratch
    ├── mining.c      # PoW, solo, pool, apply_confirmed_tx
    ├── chain.c       # init, append, retarget, verify
    ├── cli.c         # all 39 command handlers
    └── ...
```

---

## Key Design Decisions

**Coinbase-first ordering:** The mining reward transaction is added to the block *before* PoW runs. This ensures the reward is part of the Merkle root that PoW covers, preventing post-hoc reward injection.

**Union-based payloads:** The `Transaction` struct embeds a `TxPayload` union covering all 9 domain-specific payload types. No dynamic allocation is needed for transaction data.

**EVP-only cryptography:** All OpenSSL calls use the EVP abstraction layer. No legacy `EC_KEY`, `RSA`, or low-level digest APIs are used, ensuring compatibility with OpenSSL 3.0+.

**Genesis block exemption:** Block 0 is created without PoW (nonce=0). `chain_verify` explicitly skips the PoW check for `block_id == 0`, matching standard blockchain practice.

---

## Data Flow: Premium Payment

```
pay_premium alice POL001 150.0
    │
    ├── TX_PREMIUM_PAYMENT(alice → INSURANCE_POOL, 150 AHT)
    │       signed with alice's ECDSA P-256 key
    │
    └── TX_REINSURANCE_CONTRIB(INSURANCE_POOL → REINSURANCE_POOL, 7.5 AHT)
            auto-generated (5% of 150)

mine_solo alice
    │
    ├── Merkle root computed over all txs + coinbase
    ├── PoW loop until hash starts with "00..."
    ├── apply_confirmed_tx():
    │       account_debit(alice, 150)
    │       account_credit(INSURANCE_POOL, 150)
    │       utxo_create(INSURANCE_POOL, 150, tx_id, 0)
    │       reinsurance_contribute(7.5)
    │       utxo_create(REINSURANCE_POOL, 7.5, rtx_id, 0)
    └── chain_append_block()
```

---

## Verification Output

```
=== Blockchain Verification ===
  Block #0     [hash OK][pow OK][merkle OK][sigs OK]  link=OK  ts=OK  -> VALID
  Block #1     [hash OK][pow OK][merkle OK][sigs OK]  link=OK  ts=OK  -> VALID
  Block #2     [hash OK][pow OK][merkle OK][sigs OK]  link=OK  ts=OK  -> VALID
  Block #3     [hash OK][pow OK][merkle OK][sigs OK]  link=OK  ts=OK  -> VALID
=== Result: CHAIN VALID ===
```

Each block is checked for: SHA-256 hash consistency, PoW difficulty compliance, Merkle root integrity, and ECDSA signature validity on every participant-signed transaction.
