#include "cli.h"
#include "mining.h"
#include "persistence.h"
#include "participants.h"
#include "utxo.h"
#include "account.h"
#include "insurance.h"
#include "fraud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── cli.c — TODO: implement each function ─────────────────────────────
 * All function signatures match the declarations in include/cli.h.
 * Fill in one function at a time; the project compiles at every stage.
 * ─────────────────────────────────────────────────────────────────────── */


/* TODO stubs — each function compiles and returns a safe default value.
 * Replace each stub body with the real implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ARGS  16
#define MAX_LINE  512

/* ── Internal helpers ────────────────────────────────────────────────────── */

int cli_tokenise(char *line, char **argv, int max_args) {
    int argc = 0;
    char *token = strtok(line, " \t\n\r");
    while (token && argc < max_args) { argv[argc++] = token; token = strtok(NULL, " \t\n\r"); }
    return argc;
}

void cli_help(void) {
    printf(
      "\n=== ALU Health Insurance Blockchain CLI ===\n"
      "Membership    : register_member <id> <name>\n"
      "              : view_member <id>  |  wallet_balance <id>\n"
      "Policy        : enroll_policy <member_id> <policy_id> <plan> <premium>\n"
      "              : view_policy <id>  |  renew_policy <id>  |  policy_status <id>\n"
      "Token         : token_transfer <from> <to> <amount>  |  token_balance <addr>\n"
      "Insurance     : pay_premium <member_id> <policy_id> <amount>\n"
      "              : service_request <member_id> <provider_id> <type>\n"
      "              : preauth_request <preauth_id> <member_id> <prov_id> <type> <amt>\n"
      "              : preauth_approve <preauth_id> <approved_amt>\n"
      "              : submit_claim <claim_id> <member_id> <policy_id> <prov_id> <amt>\n"
      "              : approve_claim <claim_id> <approved_amt>\n"
      "              : reject_claim <claim_id> <reason>\n"
      "              : settle_claim <claim_id>\n"
      "              : reinsurance_balance\n"
      "Blockchain    : create_transaction  |  mempool_view\n"
      "              : mine_solo <miner_id>  |  mine_pool <miner1_id> [miner2_id ...]\n"
      "              : blockchain_view  |  blockchain_verify\n"
      "              : chain_save [path]  |  chain_load <path>\n"
      "              : difficulty_status\n"
      "UTXO          : utxo_view [address]  |  utxo_validate <utxo_id> <address>\n"
      "Account       : account_balance <address>  |  account_nonce <address>\n"
      "              : account_transfer <from> <to> <amount>\n"
      "Fraud/Audit   : fraud_review\n"
      "              : approve_suspicious <tx_id>  |  reject_suspicious <tx_id>\n"
      "              : transaction_history  |  provider_history <prov_id>\n"
      "              : premium_history <member_id>  |  claim_history <member_id>\n"
      "System        : help  |  exit\n\n");
}

/* ── Command dispatch table ──────────────────────────────────────────────── */

typedef struct { const char *name; int (*fn)(Blockchain *, char **, int); } Cmd;

static Cmd CMD_TABLE[] = {
    {"register_member",    cmd_register_member  },
    {"register_provider",  cmd_register_provider},
    {"view_provider",      cmd_view_provider    },
    {"view_member",        cmd_view_member      },
    {"wallet_balance",     cmd_wallet_balance   },
    {"enroll_policy",      cmd_enroll_policy    },
    {"view_policy",        cmd_view_policy      },
    {"renew_policy",       cmd_renew_policy     },
    {"policy_status",      cmd_policy_status    },
    {"token_transfer",     cmd_token_transfer   },
    {"token_balance",      cmd_token_balance    },
    {"pay_premium",        cmd_pay_premium      },
    {"service_request",    cmd_service_request  },
    {"preauth_request",    cmd_preauth_request  },
    {"preauth_approve",    cmd_preauth_approve  },
    {"submit_claim",       cmd_submit_claim     },
    {"approve_claim",      cmd_approve_claim    },
    {"reject_claim",       cmd_reject_claim     },
    {"settle_claim",       cmd_settle_claim     },
    {"reinsurance_balance",cmd_reinsurance_bal  },
    {"create_transaction", cmd_create_transaction},
    {"mempool_view",       cmd_mempool_view     },
    {"mine_solo",          cmd_mine_solo        },
    {"mine_pool",          cmd_mine_pool        },
    {"blockchain_view",    cmd_blockchain_view  },
    {"blockchain_verify",  cmd_blockchain_verify},
    {"chain_save",         cmd_chain_save       },
    {"chain_load",         cmd_chain_load       },
    {"difficulty_status",  cmd_difficulty_status},
    {"utxo_view",          cmd_utxo_view        },
    {"utxo_validate",      cmd_utxo_validate    },
    {"account_balance",    cmd_account_balance  },
    {"account_transfer",   cmd_account_transfer },
    {"account_nonce",      cmd_account_nonce    },
    {"fraud_review",       cmd_fraud_review     },
    {"approve_suspicious", cmd_approve_suspicious},
    {"reject_suspicious",  cmd_reject_suspicious},
    {"transaction_history",cmd_transaction_history},
    {"provider_history",   cmd_provider_history },
    {"premium_history",    cmd_premium_history  },
    {"claim_history",      cmd_claim_history    },
    {NULL, NULL}
};

int cli_dispatch(Blockchain *bc, char **args, int argc) {
    if (argc == 0) return 0;
    for (int i = 0; CMD_TABLE[i].name; i++)
        if (strcmp(CMD_TABLE[i].name, args[0]) == 0)
            return CMD_TABLE[i].fn(bc, args, argc);
    printf("Unknown command: %s  (type 'help' for list)\n", args[0]);
    return -1;
}

void cli_run(Blockchain *bc) {
    char line[MAX_LINE]; char *args[MAX_ARGS];
    printf("ALU Health Blockchain CLI — type 'help' for commands\n");
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(line, MAX_LINE, stdin)) break;
        int argc = cli_tokenise(line, args, MAX_ARGS);
        if (argc == 0) continue;
        if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
            chain_save(bc);
            printf("Chain saved. Goodbye.\n");
            break;
        }
        if (strcmp(args[0], "help") == 0) { cli_help(); continue; }
        cli_dispatch(bc, args, argc);
    }
}

/* ── Command implementations (TODO: fill in each) ────────────────────────── */

int cmd_register_member(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: register_member <id> <name>\n"); return -1; }
    Member *m = member_register(&bc->members, args[1], args[2]);
    if (!m) { printf("ERROR: could not register member.\n"); return -1; }
    account_create(&bc->accounts, m->keys.wallet_address, m->name, 0.0);
    /* Give new member 1000 AHT from insurance pool (initial token allocation) */
    account_transfer(bc->accounts, ADDR_INSURANCE_POOL,
                     m->keys.wallet_address, 1000.0);
    bc->token.circulating_supply += 1000.0;
    member_print(m);
    printf("  Initial allocation: 1000.0000 AHT from Insurance Pool.\n");
    return 0;
}

int cmd_view_member(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: view_member <id>\n"); return -1; }
    Member *m = member_find(bc->members, args[1]);
    if (!m) { printf("Member not found: %s\n", args[1]); return -1; }
    member_print(m);
    return 0;
}

int cmd_wallet_balance(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: wallet_balance <member_id|provider_id>\n"); return -1; }
    Member   *m = member_find(bc->members, args[1]);
    Provider *p = provider_find(bc->providers, args[1]);
    const char *addr = m ? m->keys.wallet_address
                     : p ? p->keys.wallet_address : NULL;
    if (!addr) { printf("Member/Provider not found: %s\n", args[1]); return -1; }
    double bal = account_get_balance(bc->accounts, addr);
    printf("Wallet %s (%.16s...): %.4f AHT\n", args[1], addr, bal);
    return 0;
}

int cmd_register_provider(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: register_provider <id> <name>\n"); return -1; }
    const char *specialty = (argc >= 4) ? args[3] : "General";
    Provider *p = provider_register(&bc->providers, args[1], args[2], specialty);
    if (!p) { printf("ERROR: could not register provider.\n"); return -1; }
    account_create(&bc->accounts, p->keys.wallet_address, p->name, 0.0);
    provider_print(p);
    return 0;
}

int cmd_view_provider(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: view_provider <id>\n"); return -1; }
    Provider *p = provider_find(bc->providers, args[1]);
    if (!p) { printf("Provider not found: %s\n", args[1]); return -1; }
    provider_print(p);
    return 0;
}

int cmd_enroll_policy(Blockchain *bc, char **args, int argc) {
    if (argc < 5) { printf("Usage: enroll_policy <member_id> <policy_id> <plan> <premium>\n"); return -1; }
    Member *m = member_find(bc->members, args[1]);
    if (!m) { printf("Member not found.\n"); return -1; }
    double premium = atof(args[4]);
    Policy *p = policy_create(&bc->policies, args[2], args[1], args[3], premium, time(NULL));
    if (!p) { printf("ERROR: policy creation failed.\n"); return -1; }
    strncpy(m->active_policy, args[2], ID_SIZE - 1);

    /* Create and sign TX_POLICY_ENROLL — records the enrolment on-chain */
    Transaction *etx = tx_create(TX_POLICY_ENROLL,
                                   m->keys.wallet_address,
                                   ADDR_INSURANCE_POOL, premium);
    etx->sender_nonce = account_get_next_nonce(bc->accounts, m->keys.wallet_address);
    strncpy(etx->payload.policy_enroll.member_id,     args[1], ID_SIZE   - 1);
    strncpy(etx->payload.policy_enroll.policy_id,     args[2], ID_SIZE   - 1);
    strncpy(etx->payload.policy_enroll.coverage_plan, args[3], NAME_SIZE - 1);
    etx->payload.policy_enroll.enrollment_date = p->enrollment_date;
    etx->payload.policy_enroll.expiry_date     = p->expiry_date;
    etx->payload.policy_enroll.status   = POLICY_ACTIVE;
    tx_generate_id(etx);
    tx_sign(etx, &m->keys);
    mempool_add(&bc->mempool, etx, 1.0);

    policy_print(p);
    printf("Policy enrolled and TX_POLICY_ENROLL queued in mempool.\n");
    return 0;
}

int cmd_view_policy(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: view_policy <policy_id>\n"); return -1; }
    Policy *p = policy_find(bc->policies, args[1]);
    if (!p) { printf("Policy not found.\n"); return -1; }
    policy_check_expiry(bc->policies, args[1], time(NULL));
    policy_print(p);
    return 0;
}

int cmd_renew_policy(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: renew_policy <policy_id>\n"); return -1; }
    if (policy_renew(bc->policies, args[1], time(NULL)) < 0) {
        printf("Policy not found: %s\n", args[1]); return -1;
    }
    policy_print(policy_find(bc->policies, args[1]));
    return 0;
}

int cmd_policy_status(Blockchain *bc, char **args, int argc) {
    return cmd_view_policy(bc, args, argc);
}

int cmd_token_transfer(Blockchain *bc, char **args, int argc) {
    if (argc < 4) { printf("Usage: token_transfer <from> <to> <amount>\n"); return -1; }
    double amt = atof(args[3]);
    /* Resolve sender: accept member ID or raw wallet address */
    Member *_sender_m = member_find(bc->members, args[1]);
    const char *sender_addr = _sender_m ? _sender_m->keys.wallet_address : args[1];
    /* Resolve receiver: accept member ID or raw wallet address */
    Member *_recv_m = member_find(bc->members, args[2]);
    const char *recv_addr = _recv_m ? _recv_m->keys.wallet_address : args[2];
    /* spec 7.v: validate amount is within sender balance */
    double bal = account_get_balance(bc->accounts, sender_addr);
    if (bal < 0.0) {
        printf("ERROR: sender account not found: %s\n", args[1]); return -1;
    }
    if (amt > bal) {
        printf("ERROR: insufficient balance. Have %.4f AHT, need %.4f AHT.\n", bal, amt);
        return -1;
    }
    Transaction *tx = tx_create(TX_TOKEN_TRANSFER, sender_addr, recv_addr, amt);
    tx->sender_nonce = account_get_next_nonce(bc->accounts, sender_addr);
    tx_generate_id(tx);
    if (_sender_m) tx_sign(tx, &_sender_m->keys);
    if (mempool_add(&bc->mempool, tx, 1.0) < 0) {
        printf("ERROR: could not add to mempool.\n"); tx_free(tx); return -1;
    }
    printf("Token transfer queued in mempool. Mine to confirm.\n");
    return 0;
}

int cmd_token_balance(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: token_balance <member_id|provider_id|address>\n"); return -1; }

    /* Resolve member ID, provider ID, or raw wallet address */
    Member   *m = member_find(bc->members, args[1]);
    Provider *p = provider_find(bc->providers, args[1]);
    const char *addr = m ? m->keys.wallet_address
                     : p ? p->keys.wallet_address
                     : args[1];

    double bal = account_get_balance(bc->accounts, addr);
    if (bal < 0.0) {
        printf("Account not found: %s\n", args[1]); return -1;
    }
    printf("Token balance [%s]: %.4f AHT\n", args[1], bal);
    return 0;
}

int cmd_pay_premium(Blockchain *bc, char **args, int argc) {
    if (argc < 4) { printf("Usage: pay_premium <member_id> <policy_id> <amount>\n"); return -1; }
    Member *m = member_find(bc->members, args[1]);
    if (!m) { printf("Member not found.\n"); return -1; }
    if (!policy_find(bc->policies, args[2])) { printf("Policy not found.\n"); return -1; }
    double amt = atof(args[3]);
    /* spec 7.v: validate member has sufficient balance for the premium */
    double bal = account_get_balance(bc->accounts, m->keys.wallet_address);
    if (amt > bal) {
        printf("ERROR: insufficient balance. Have %.4f AHT, need %.4f AHT.\n", bal, amt);
        return -1;
    }
    /* Main premium tx */
    Transaction *tx = tx_create(TX_PREMIUM_PAYMENT, m->keys.wallet_address,
                                  ADDR_INSURANCE_POOL, amt);
    tx->sender_nonce = account_get_next_nonce(bc->accounts, m->keys.wallet_address);
    strncpy(tx->payload.premium.policy_id, args[2], ID_SIZE - 1);
    strncpy(tx->payload.premium.member_id, args[1], ID_SIZE - 1);
    tx_generate_id(tx);
    tx_sign(tx, &m->keys);             /* member signs their premium payment */
    mempool_add(&bc->mempool, tx, 1.0);
    /* Auto reinsurance contribution (5%) — system-generated, not signed */
    double contrib = amt * REINSURANCE_RATE;
    Transaction *rtx = tx_create(TX_REINSURANCE_CONTRIBUTION, ADDR_INSURANCE_POOL,
                                   ADDR_REINSURANCE_POOL, contrib);
    rtx->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_INSURANCE_POOL);
    strncpy(rtx->payload.reinsurance_contribution.source_tx_id, tx->transaction_id, HASH_SIZE - 1);
    tx_generate_id(rtx);
    mempool_add(&bc->mempool, rtx, 1.0);
    printf("Premium %.4f AHT queued. Reinsurance contribution %.4f AHT queued. Mine to confirm.\n",
           amt, contrib);
    return 0;
}

int cmd_service_request(Blockchain *bc, char **args, int argc) {
    if (argc < 4) { printf("Usage: service_request <member_id> <provider_id> <type>\n"); return -1; }
    Member   *m = member_find(bc->members, args[1]);
    Provider *p = provider_find(bc->providers, args[2]);
    if (!m) { printf("Member not found.\n");   return -1; }
    if (!p) { printf("Provider not found.\n"); return -1; }
    Transaction *tx = tx_create(TX_SERVICE_REQUEST, m->keys.wallet_address,
                                  p->keys.wallet_address, 0.0);
    strncpy(tx->payload.service.member_id,   args[1], ID_SIZE   - 1);
    strncpy(tx->payload.service.provider_id, args[2], ID_SIZE   - 1);
    strncpy(tx->payload.service.service_type,args[3], NAME_SIZE - 1);
    tx->sender_nonce = account_get_next_nonce(bc->accounts, m->keys.wallet_address);
    tx_generate_id(tx);
    tx_sign(tx, &m->keys);
    mempool_add(&bc->mempool, tx, 0.5);
    printf("Service request queued. Mine to confirm.\n");
    return 0;
}

int cmd_preauth_request(Blockchain *bc, char **args, int argc) {
    if (argc < 6) { printf("Usage: preauth_request <preauth_id> <member_id> <prov_id> <type> <est_cost>\n"); return -1; }
    preauth_create(&bc->preauths, args[1], args[2], args[3], args[4], atof(args[5]));

    /* Record TX_PREAUTH_REQUEST on-chain so it's auditable */
    Member *pm = member_find(bc->members, args[2]);
    if (pm) {
        Transaction *ptx = tx_create(TX_PREAUTH_REQUEST,
                                      pm->keys.wallet_address,
                                      ADDR_INSURANCE_POOL, atof(args[5]));
        ptx->sender_nonce = account_get_next_nonce(bc->accounts, pm->keys.wallet_address);
        strncpy(ptx->payload.preauth_request.preauth_id,   args[1], ID_SIZE   - 1);
        strncpy(ptx->payload.preauth_request.member_id,    args[2], ID_SIZE   - 1);
        strncpy(ptx->payload.preauth_request.provider_id,  args[3], ID_SIZE   - 1);
        strncpy(ptx->payload.preauth_request.service_type, args[4], NAME_SIZE - 1);
        ptx->payload.preauth_request.estimated_cost = atof(args[5]);
        tx_generate_id(ptx);
        tx_sign(ptx, &pm->keys);
        mempool_add(&bc->mempool, ptx, 0.5);
        printf("Pre-auth request created: %s — TX_PREAUTH_REQUEST queued.\n", args[1]);
    } else {
        printf("Pre-auth request created: %s (member not found, not going on-chain)\n", args[1]);
    }
    return 0;
}

int cmd_preauth_approve(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: preauth_approve <preauth_id> <approved_amt>\n"); return -1; }
    double approved_amt = atof(args[2]);

    /* Update domain object */
    if (preauth_approve(bc->preauths, args[1], approved_amt) < 0) {
        printf("Pre-auth not found or not pending.\n"); return -1;
    }

    /* Spec Section 2.v: record the approval as TX_PREAUTH_APPROVE on-chain */
    Preauth *pa = preauth_find(bc->preauths, args[1]);
    Transaction *tx = tx_create(TX_PREAUTH_APPROVE,
                                 ADDR_INSURANCE_POOL,
                                 pa ? pa->member_id : args[1],
                                 approved_amt);
    if (!tx) { printf("ERROR: could not allocate transaction.\n"); return -1; }
    strncpy(tx->payload.preauth_approve.preauth_id, args[1], ID_SIZE - 1);
    tx->payload.preauth_approve.approved_amount = approved_amt;
    tx->payload.preauth_approve.status          = PREAUTH_APPROVED;
    tx->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_INSURANCE_POOL);
    tx_generate_id(tx);
    if (mempool_add(&bc->mempool, tx, 1.5) < 0) {
        printf("ERROR: could not queue TX_PREAUTH_APPROVE.\n"); tx_free(tx); return -1;
    }
    printf("Pre-auth %s approved for %.2f AHT — TX_PREAUTH_APPROVE queued. Mine to confirm.\n",
           args[1], approved_amt);
    return 0;
}

int cmd_submit_claim(Blockchain *bc, char **args, int argc) {
    if (argc < 6) { printf("Usage: submit_claim <claim_id> <member_id> <policy_id> <prov_id> <amount>\n"); return -1; }
    time_t now = time(NULL);
    /* 1. Policy expiry check */
    if (!policy_is_claimable(bc->policies, args[3], now)) {
        printf("REJECTED: policy %s is expired or does not exist.\n", args[3]); return -1;
    }
    Member   *m = member_find(bc->members, args[2]);
    Provider *p = provider_find(bc->providers, args[4]);
    if (!m || !p) { printf("Member or provider not found.\n"); return -1; }
    double amt = atof(args[5]);

    /* 2. Fraud checks */
    int claim_24h    = claim_count_in_window(bc->claims, args[2], now);
    double prov_avg  = claim_provider_average(bc->claims, args[4]);
    char tx_ids[1024][HASH_SIZE];
    uint32_t id_cnt = chain_collect_tx_ids(bc, tx_ids, 1024);
    Transaction *tx = tx_create(TX_CLAIM_SUBMIT, p->keys.wallet_address,
                                  ADDR_INSURANCE_POOL, amt);
    strncpy(tx->payload.claim_submit.claim_id,    args[1], ID_SIZE - 1);
    strncpy(tx->payload.claim_submit.member_id,   args[2], ID_SIZE - 1);
    strncpy(tx->payload.claim_submit.policy_id,   args[3], ID_SIZE - 1);
    strncpy(tx->payload.claim_submit.provider_id, args[4], ID_SIZE - 1);
    tx->sender_nonce = account_get_next_nonce(bc->accounts, p->keys.wallet_address);
    tx_generate_id(tx);
    tx_sign(tx, &p->keys);   /* provider signs the claim submission */

    FraudFlag flags = fraud_check_transaction(tx, claim_24h, prov_avg,
                                               (const char (*)[HASH_SIZE])tx_ids, id_cnt);
    if (flags != FRAUD_CLEAN) {
        printf("FRAUD DETECTED (flags=0x%x): claim flagged SUSPICIOUS and withheld.\n", flags);
        mempool_add(&bc->mempool, tx, 1.0);
        mempool_mark_suspicious(bc->mempool, tx->transaction_id);
        fraud_log_add(&bc->fraud_log, tx, flags, "auto-flagged during submission");
        claim_create(&bc->claims, args[1], args[2], args[3], args[4], amt, now, "");
        return 0;
    }
    mempool_add(&bc->mempool, tx, 1.0);
    claim_create(&bc->claims, args[1], args[2], args[3], args[4], amt, now, "");
    printf("Claim %s submitted. Mine to confirm.\n", args[1]);
    return 0;
}

int cmd_approve_claim(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: approve_claim <claim_id> <approved_amt>\n"); return -1; }
    double approved_amt = atof(args[2]);

    /* Update domain object */
    if (claim_approve(bc->claims, args[1], approved_amt) < 0) {
        printf("Claim not found or not pending.\n"); return -1;
    }

    /* Spec Section 2.vii: record the approval decision as a TX_CLAIM_APPROVE
     * on-chain so the insurer's decision is auditable on the blockchain.    */
    Claim *cl = claim_find(bc->claims, args[1]);
    Transaction *tx = tx_create(TX_CLAIM_APPROVE,
                                 ADDR_INSURANCE_POOL,
                                 cl ? cl->member_id : args[1],
                                 approved_amt);
    if (!tx) { printf("ERROR: could not allocate transaction.\n"); return -1; }
    strncpy(tx->payload.claim_decision.claim_id,       args[1], ID_SIZE   - 1);
    tx->payload.claim_decision.approved_amount = approved_amt;
    tx->payload.claim_decision.decision        = CLAIM_APPROVED;
    tx->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_INSURANCE_POOL);
    tx_generate_id(tx);
    if (mempool_add(&bc->mempool, tx, 2.0) < 0) {
        printf("ERROR: could not queue TX_CLAIM_APPROVE.\n"); tx_free(tx); return -1;
    }
    printf("Claim %s approved for %.2f AHT — TX_CLAIM_APPROVE queued. Mine to confirm.\n",
           args[1], approved_amt);
    return 0;
}

int cmd_reject_claim(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: reject_claim <claim_id> <reason>\n"); return -1; }

    /* Update domain object */
    claim_reject(bc->claims, args[1], args[2]);

    /* Spec Section 2.vii: record the rejection decision as TX_CLAIM_REJECT
     * on-chain so the refusal is permanently auditable.                    */
    Claim *cl = claim_find(bc->claims, args[1]);
    Transaction *tx = tx_create(TX_CLAIM_REJECT,
                                 ADDR_INSURANCE_POOL,
                                 cl ? cl->member_id : args[1],
                                 0.0);
    if (!tx) { printf("ERROR: could not allocate transaction.\n"); return -1; }
    strncpy(tx->payload.claim_decision.claim_id, args[1], ID_SIZE   - 1);
    strncpy(tx->payload.claim_decision.reason,   args[2], DESC_SIZE - 1);
    tx->payload.claim_decision.decision   = CLAIM_REJECTED;
    tx->payload.claim_decision.approved_amount = 0.0;
    tx->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_INSURANCE_POOL);
    tx_generate_id(tx);
    if (mempool_add(&bc->mempool, tx, 2.0) < 0) {
        printf("ERROR: could not queue TX_CLAIM_REJECT.\n"); tx_free(tx); return -1;
    }
    printf("Claim %s rejected — TX_CLAIM_REJECT queued. Mine to confirm.\n", args[1]);
    return 0;
}

int cmd_settle_claim(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: settle_claim <claim_id>\n"); return -1; }
    Claim *c = claim_find(bc->claims, args[1]);
    if (!c || c->status != CLAIM_APPROVED) { printf("Claim not found or not approved.\n"); return -1; }
    double ins_pay, rein_pay;
    reinsurance_compute_split(c->approved_amount, &ins_pay, &rein_pay);
    Provider *p = provider_find(bc->providers, c->provider_id);
    if (p) {
        /* Insurance pool pays first */
        Transaction *t1 = tx_create(TX_CLAIM_SETTLE, ADDR_INSURANCE_POOL,
                                     p->keys.wallet_address, ins_pay);
        t1->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_INSURANCE_POOL);
        tx_generate_id(t1);
        mempool_add(&bc->mempool, t1, 2.0);
        /* Reinsurance pays excess if any */
        if (rein_pay > 0.0) {
            double actual = reinsurance_disburse(&bc->reinsurance, rein_pay);
            if (actual < rein_pay) {
                /* spec 3.iii: partial coverage must be flagged for manual review */
                char _note[DESC_SIZE];
                snprintf(_note, DESC_SIZE,
                         "Reinsurance shortfall: needed %.2f AHT, paid %.2f AHT for claim %s",
                         rein_pay, actual, c->claim_id);
                printf("WARNING: %s\n", _note);
                /* Create a placeholder tx for the log entry */
                Transaction *_log_tx = tx_create(TX_CLAIM_SETTLE,
                                                  ADDR_REINSURANCE_POOL,
                                                  p->keys.wallet_address, actual);
                strncpy(_log_tx->payload.claim_settle.claim_id,
                        c->claim_id, ID_SIZE - 1);
                tx_generate_id(_log_tx);
                fraud_log_add(&bc->fraud_log, _log_tx,
                              FRAUD_PARTIAL_REINSURANCE, _note);
                tx_free(_log_tx);
            }
            Transaction *t2 = tx_create(TX_CLAIM_SETTLE, ADDR_REINSURANCE_POOL,
                                         p->keys.wallet_address, actual);
            t2->sender_nonce = account_get_next_nonce(bc->accounts, ADDR_REINSURANCE_POOL);
            tx_generate_id(t2);
            mempool_add(&bc->mempool, t2, 2.0);
        }
    }
    claim_settle(bc->claims, args[1]);
    printf("Settlement queued: ins=%.2f AHT  reinsuarance=%.2f AHT. Mine to confirm.\n",
           ins_pay, rein_pay);
    return 0;
}

int cmd_reinsurance_bal(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc;
    reinsurance_print(&bc->reinsurance);
    return 0;
}

int cmd_create_transaction(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc;

    char buf[256];

    printf("\n=== Create Transaction ===\n");
    printf("Select transaction type:\n");
    printf("  1  TOKEN_TRANSFER       — send AHT between wallets\n");
    printf("  2  PREMIUM_PAYMENT      — pay insurance premium\n");
    printf("  3  SERVICE_REQUEST      — record a healthcare visit\n");
    printf("  4  CLAIM_SUBMIT         — submit a claim (provider)\n");
    printf("  5  PREAUTH_REQUEST      — request pre-authorisation\n");
    printf("  6  CLAIM_APPROVE        — approve a pending claim\n");
    printf("  7  CLAIM_REJECT         — reject a pending claim\n");
    printf("  0  Cancel\n");
    printf("Choice: ");
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    int choice = atoi(buf);
    if (choice == 0) { printf("Cancelled.\n"); return 0; }

    /* ── Gather sender ───────────────────────────────────────────────── */
    char sender_id[ID_SIZE] = {0};
    printf("Sender member/provider ID: ");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    buf[strcspn(buf, "\r\n")] = 0;
    strncpy(sender_id, buf, ID_SIZE - 1);

    /* Resolve sender key */
    Member   *sm = member_find(bc->members, sender_id);
    Provider *sp = provider_find(bc->providers, sender_id);
    const char *sender_addr = sm ? sm->keys.wallet_address
                            : sp ? sp->keys.wallet_address
                            : sender_id;   /* fallback: treat ID as raw address */

    /* ── Gather receiver ─────────────────────────────────────────────── */
    char receiver_addr[ADDR_SIZE] = {0};
    printf("Receiver address or member/provider ID: ");
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    buf[strcspn(buf, "\r\n")] = 0;
    /* Check if the user typed a member/provider ID instead of raw address */
    Member   *rm = member_find(bc->members, buf);
    Provider *rp = provider_find(bc->providers, buf);
    if (rm)      strncpy(receiver_addr, rm->keys.wallet_address, ADDR_SIZE - 1);
    else if (rp) strncpy(receiver_addr, rp->keys.wallet_address, ADDR_SIZE - 1);
    else         strncpy(receiver_addr, buf, ADDR_SIZE - 1);

    /* ── Gather amount ───────────────────────────────────────────────── */
    double amount = 0.0;
    if (choice != 3 && choice != 5 && choice != 6 && choice != 7) {
        printf("Amount (AHT): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        amount = atof(buf);
    }

    /* ── Build the transaction ───────────────────────────────────────── */
    TransactionType ttype;
    switch (choice) {
        case 1: ttype = TX_TOKEN_TRANSFER;  break;
        case 2: ttype = TX_PREMIUM_PAYMENT; break;
        case 3: ttype = TX_SERVICE_REQUEST; break;
        case 4: ttype = TX_CLAIM_SUBMIT;    break;
        case 5: ttype = TX_PREAUTH_REQUEST; break;
        case 6: ttype = TX_CLAIM_APPROVE;   break;
        case 7: ttype = TX_CLAIM_REJECT;    break;
        default:
            printf("Invalid choice.\n"); return -1;
    }

    Transaction *tx = tx_create(ttype, sender_addr, receiver_addr, amount);
    if (!tx) { printf("ERROR: could not allocate transaction.\n"); return -1; }
    tx->sender_nonce = account_get_next_nonce(bc->accounts, sender_addr);

    /* ── Type-specific payload fields ────────────────────────────────── */
    if (choice == 2) {
        printf("Policy ID: "); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.premium.policy_id, buf, ID_SIZE - 1);
        strncpy(tx->payload.premium.member_id, sender_id, ID_SIZE - 1);
    } else if (choice == 3) {
        printf("Provider ID: "); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.service.provider_id, buf, ID_SIZE - 1);
        printf("Service type: "); fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.service.service_type, buf, NAME_SIZE - 1);
        strncpy(tx->payload.service.member_id, sender_id, ID_SIZE - 1);
    } else if (choice == 4) {
        printf("Claim ID: ");    fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.claim_submit.claim_id, buf, ID_SIZE - 1);
        printf("Policy ID: ");   fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.claim_submit.policy_id, buf, ID_SIZE - 1);
        printf("Diagnosis: ");   fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.claim_submit.diagnosis, buf, DESC_SIZE - 1);
        strncpy(tx->payload.claim_submit.member_id,   sender_id,   ID_SIZE - 1);
        strncpy(tx->payload.claim_submit.provider_id, receiver_addr, ID_SIZE - 1);
        tx->payload.claim_submit.claim_status = CLAIM_PENDING;
    } else if (choice == 5) {
        printf("Preauth ID: ");      fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.preauth_request.preauth_id, buf, ID_SIZE - 1);
        printf("Service type: ");    fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.preauth_request.service_type, buf, NAME_SIZE - 1);
        printf("Estimated cost: ");  fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        tx->payload.preauth_request.estimated_cost = atof(buf);
        strncpy(tx->payload.preauth_request.member_id,   sender_id,   ID_SIZE - 1);
        strncpy(tx->payload.preauth_request.provider_id, receiver_addr, ID_SIZE - 1);
    } else if (choice == 6 || choice == 7) {
        printf("Claim ID: ");  fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
        buf[strcspn(buf, "\r\n")] = 0;
        strncpy(tx->payload.claim_decision.claim_id, buf, ID_SIZE - 1);
        if (choice == 6) {
            printf("Approved amount: "); fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
            tx->payload.claim_decision.approved_amount = atof(buf);
            tx->payload.claim_decision.decision = CLAIM_APPROVED;
        } else {
            printf("Rejection reason: "); fflush(stdout);
            if (!fgets(buf, sizeof(buf), stdin)) { tx_free(tx); return 0; }
            buf[strcspn(buf, "\r\n")] = 0;
            strncpy(tx->payload.claim_decision.reason, buf, DESC_SIZE - 1);
            tx->payload.claim_decision.decision = CLAIM_REJECTED;
        }
    }

    /* ── Sign and submit ─────────────────────────────────────────────── */
    tx_generate_id(tx);
    if (sm)      tx_sign(tx, &sm->keys);
    else if (sp) tx_sign(tx, &sp->keys);

    if (mempool_add(&bc->mempool, tx, 1.0) < 0) {
        printf("ERROR: transaction rejected by mempool.\n");
        tx_free(tx);
        return -1;
    }

    printf("\nTransaction created and queued:\n");
    tx_print(tx);
    printf("Mine a block to confirm it.\n\n");
    return 0;
}

int cmd_mempool_view(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc;
    mempool_print(bc->mempool); return 0;
}

int cmd_mine_solo(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: mine_solo <miner_id>\n"); return -1; }
    Member *m = member_find(bc->members, args[1]);
    const char *addr = m ? m->keys.wallet_address : args[1];
    mine_solo(bc, args[1], addr);
    /* Print UTXO set after block confirmation per spec */
    utxo_print_all(bc->utxo_set);
    return 0;
}

int cmd_mine_pool(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: mine_pool <miner1> [miner2 ...]\n"); return -1; }
    PoolSession *s = mine_pool_init();
    for (int i = 1; i < argc; i++) {
        Member *m = member_find(bc->members, args[i]);
        const char *addr = m ? m->keys.wallet_address : args[i];
        mine_pool_add(s, args[i], addr);
    }
    mine_pool_run(bc, s);
    mine_pool_free(s);
    utxo_print_all(bc->utxo_set);
    return 0;
}

int cmd_blockchain_view(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc; chain_print(bc); return 0;
}

int cmd_blockchain_verify(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc; chain_verify(bc); return 0;
}

int cmd_chain_save(Blockchain *bc, char **args, int argc) {
    const char *path = (argc >= 2) ? args[1] : bc->state.save_file;
    return chain_save_to(bc, path);
}

int cmd_chain_load(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: chain_load <path>\n"); return -1; }

    printf("[chain_load] Loading from %s...\n", args[1]);
    Blockchain *loaded = chain_load(args[1]);
    if (!loaded) {
        printf("ERROR: could not load chain from '%s'. File missing or corrupt.\n", args[1]);
        return -1;
    }

    /* Verify the loaded chain before replacing current state */
    printf("[chain_load] Verifying loaded chain...\n");
    if (!chain_verify(loaded)) {
        printf("ERROR: loaded chain failed verification. Current chain unchanged.\n");
        chain_free(loaded);
        return -1;
    }

    /* Free all dynamic members of current bc (but NOT bc itself — it is
     * owned by cli_run's caller and must stay at the same address).     */
    Block *b = bc->genesis, *nxt;
    for (; b; b = nxt) { nxt = b->next; block_free(b); }
    mempool_free_all(&bc->mempool);
    utxo_free_all(&bc->utxo_set);
    account_free_all(&bc->accounts);
    member_free_all(&bc->members);
    provider_free_all(&bc->providers);
    policy_free_all(&bc->policies);
    claim_free_all(&bc->claims);
    preauth_free_all(&bc->preauths);
    fraud_log_free_all(&bc->fraud_log);

    /* Shallow-copy loaded state into bc — all pointers transfer ownership */
    *bc = *loaded;

    /* Free only the shell of 'loaded'; its members are now owned by bc */
    free(loaded);

    printf("[chain_load] Chain loaded. Height=%u  Difficulty=%u  Reward=%.2f AHT\n",
           bc->state.height, bc->state.difficulty, bc->state.block_reward);
    return 0;
}

int cmd_difficulty_status(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc;
    printf("Current difficulty    : %u\n", bc->state.difficulty);
    printf("Block reward          : %.4f AHT\n", bc->state.block_reward);
    printf("Last retarget block   : %u\n", bc->state.last_retarget_block);
    printf("Chain height          : %u\n", bc->state.height);
    printf("Next retarget at block: %u\n",
           bc->state.last_retarget_block + BLOCKS_PER_RETARGET);
    return 0;
}

int cmd_utxo_view(Blockchain *bc, char **args, int argc) {
    if (argc >= 2) utxo_print_by_owner(bc->utxo_set, args[1]);
    else            utxo_print_all(bc->utxo_set);
    return 0;
}

int cmd_utxo_validate(Blockchain *bc, char **args, int argc) {
    if (argc < 3) { printf("Usage: utxo_validate <utxo_id> <owner_address>\n"); return -1; }
    int r = utxo_validate_spend(bc->utxo_set, args[1], args[2]);
    printf("UTXO %s for %s: %s\n", args[1], args[2],
           r == 1 ? "VALID (unspent, correct owner)" :
           r == 0 ? "INVALID (spent or wrong owner)" : "NOT FOUND");
    return 0;
}

int cmd_account_balance(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: account_balance <address>\n"); return -1; }
    double bal = account_get_balance(bc->accounts, args[1]);
    printf("Account [%.16s...]: %.4f AHT\n", args[1], bal);
    return 0;
}

int cmd_account_transfer(Blockchain *bc, char **args, int argc) {
    if (argc < 4) { printf("Usage: account_transfer <from> <to> <amount>\n"); return -1; }
    if (account_transfer(bc->accounts, args[1], args[2], atof(args[3])) < 0) {
        printf("Transfer failed (insufficient funds or account not found).\n"); return -1;
    }
    printf("Account transfer: %.4f AHT from %s to %s\n", atof(args[3]), args[1], args[2]);
    return 0;
}

int cmd_account_nonce(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: account_nonce <address>\n"); return -1; }
    Account *a = account_find(bc->accounts, args[1]);
    if (!a) { printf("Account not found.\n"); return -1; }
    printf("Account [%.16s...] nonce: %llu\n", args[1], (unsigned long long)a->nonce);
    return 0;
}

int cmd_fraud_review(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc; fraud_log_print(bc->fraud_log); return 0;
}

int cmd_approve_suspicious(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: approve_suspicious <tx_id>\n"); return -1; }
    if (mempool_approve_suspicious(bc->mempool, args[1]) < 0) {
        printf("TX not found or not SUSPICIOUS.\n"); return -1;
    }
    fraud_log_remove(&bc->fraud_log, args[1]);
    printf("TX %.16s... approved: status set to PENDING.\n", args[1]);
    return 0;
}

int cmd_reject_suspicious(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: reject_suspicious <tx_id>\n"); return -1; }
    if (mempool_reject_suspicious(&bc->mempool, args[1]) < 0) {
        printf("TX not found or not SUSPICIOUS.\n"); return -1;
    }
    fraud_log_remove(&bc->fraud_log, args[1]);
    printf("TX %.16s... rejected and removed.\n", args[1]);
    return 0;
}

int cmd_transaction_history(Blockchain *bc, char **args, int argc) {
    (void)args; (void)argc;
    printf("=== Full Transaction History ===\n");
    for (Block *b = bc->genesis; b; b = b->next)
        for (uint32_t i = 0; i < b->transaction_count; i++)
            tx_print(b->transactions[i]);
    return 0;
}

int cmd_provider_history(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: provider_history <provider_id>\n"); return -1; }
    /* Resolve provider ID to wallet address for on-chain lookup */
    Provider *pv = provider_find(bc->providers, args[1]);
    const char *paddr = pv ? pv->keys.wallet_address : args[1];

    printf("=== On-chain transaction history for provider %s ===\n", args[1]);
    int found = 0;
    for (Block *b = bc->genesis; b; b = b->next) {
        for (uint32_t i = 0; i < b->transaction_count; i++) {
            Transaction *tx = b->transactions[i];
            /* Show any tx sent FROM or TO this provider */
            if (strncmp(tx->sender_address,   paddr, ADDR_SIZE) == 0 ||
                strncmp(tx->receiver_address, paddr, ADDR_SIZE) == 0) {
                tx_print(tx);
                found++;
            }
        }
    }
    if (!found) printf("  (no confirmed transactions for this provider)\n");
    /* Also show domain-layer claim records for completeness */
    claim_print_by_provider(bc->claims, args[1]);
    return 0;
}

int cmd_premium_history(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: premium_history <member_id>\n"); return -1; }
    printf("=== Premium history for %s ===\n", args[1]);
    for (Block *b = bc->genesis; b; b = b->next)
        for (uint32_t i = 0; i < b->transaction_count; i++)
            if (b->transactions[i]->transaction_type == TX_PREMIUM_PAYMENT &&
                strncmp(b->transactions[i]->payload.premium.member_id,
                        args[1], ID_SIZE) == 0)
                tx_print(b->transactions[i]);
    return 0;
}

int cmd_claim_history(Blockchain *bc, char **args, int argc) {
    if (argc < 2) { printf("Usage: claim_history <member_id>\n"); return -1; }
    Member *m = member_find(bc->members, args[1]);
    const char *maddr = m ? m->keys.wallet_address : args[1];

    printf("=== On-chain claim transactions for member %s ===\n", args[1]);
    int found = 0;
    for (Block *b = bc->genesis; b; b = b->next) {
        for (uint32_t i = 0; i < b->transaction_count; i++) {
            Transaction *tx = b->transactions[i];
            /* Show all claim-related transactions involving this member */
            int is_claim_tx = (tx->transaction_type == TX_CLAIM_SUBMIT  ||
                               tx->transaction_type == TX_CLAIM_APPROVE ||
                               tx->transaction_type == TX_CLAIM_REJECT  ||
                               tx->transaction_type == TX_CLAIM_SETTLE);
            int involves_member = (
                strncmp(tx->sender_address,   maddr, ADDR_SIZE) == 0 ||
                strncmp(tx->receiver_address, maddr, ADDR_SIZE) == 0 ||
                strncmp(tx->receiver_address, args[1], ID_SIZE) == 0);
            if (is_claim_tx && involves_member) {
                tx_print(tx);
                found++;
            }
        }
    }
    if (!found) printf("  (no confirmed claim transactions for this member)\n");
    /* Also show domain-layer claim records */
    claim_print_by_member(bc->claims, args[1]);
    return 0;
}
