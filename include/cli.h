#ifndef CLI_H
#define CLI_H

#include "chain.h"

/*
 * Each handler receives the global Blockchain pointer and an argv-style
 * argument array (args[0] = command name, args[1..] = parameters).
 * Returns 0 on success, non-zero on error.
 */

/* ── Main dispatch ───────────────────────────────────────────────────────── */

/* cli_run — read-evaluate loop; returns when user types "exit" */
void cli_run(Blockchain *bc);

/* cli_dispatch — look up cmd in the command table and call the handler */
int  cli_dispatch(Blockchain *bc, char **args, int argc);

/* cli_help — print all available commands */
void cli_help(void);

/* ── Membership ──────────────────────────────────────────────────────────── */
int cmd_register_member  (Blockchain *bc, char **args, int argc);
int cmd_view_member      (Blockchain *bc, char **args, int argc);
int cmd_wallet_balance   (Blockchain *bc, char **args, int argc);

/* ── Provider ───────────────────────────────────────────────────────────── */
int cmd_register_provider (Blockchain *bc, char **args, int argc);
int cmd_view_provider     (Blockchain *bc, char **args, int argc);

/* ── Policy ──────────────────────────────────────────────────────────────── */
int cmd_enroll_policy    (Blockchain *bc, char **args, int argc);
int cmd_view_policy      (Blockchain *bc, char **args, int argc);
int cmd_renew_policy     (Blockchain *bc, char **args, int argc);
int cmd_policy_status    (Blockchain *bc, char **args, int argc);

/* ── Token ───────────────────────────────────────────────────────────────── */
int cmd_token_transfer   (Blockchain *bc, char **args, int argc);
int cmd_token_balance    (Blockchain *bc, char **args, int argc);

/* ── Insurance operations ────────────────────────────────────────────────── */
int cmd_pay_premium      (Blockchain *bc, char **args, int argc);
int cmd_service_request  (Blockchain *bc, char **args, int argc);
int cmd_preauth_request  (Blockchain *bc, char **args, int argc);
int cmd_preauth_approve  (Blockchain *bc, char **args, int argc);
int cmd_submit_claim     (Blockchain *bc, char **args, int argc);
int cmd_approve_claim    (Blockchain *bc, char **args, int argc);
int cmd_reject_claim     (Blockchain *bc, char **args, int argc);
int cmd_settle_claim     (Blockchain *bc, char **args, int argc);
int cmd_reinsurance_bal  (Blockchain *bc, char **args, int argc);

/* ── Blockchain ──────────────────────────────────────────────────────────── */
int cmd_create_transaction(Blockchain *bc, char **args, int argc);
int cmd_mempool_view      (Blockchain *bc, char **args, int argc);
int cmd_mine_solo         (Blockchain *bc, char **args, int argc);
int cmd_mine_pool         (Blockchain *bc, char **args, int argc);
int cmd_blockchain_view   (Blockchain *bc, char **args, int argc);
int cmd_blockchain_verify (Blockchain *bc, char **args, int argc);
int cmd_chain_save        (Blockchain *bc, char **args, int argc);
int cmd_chain_load        (Blockchain *bc, char **args, int argc);
int cmd_difficulty_status (Blockchain *bc, char **args, int argc);

/* ── UTXO ────────────────────────────────────────────────────────────────── */
int cmd_utxo_view        (Blockchain *bc, char **args, int argc);
int cmd_utxo_validate    (Blockchain *bc, char **args, int argc);

/* ── Account model ───────────────────────────────────────────────────────── */
int cmd_account_balance  (Blockchain *bc, char **args, int argc);
int cmd_account_transfer (Blockchain *bc, char **args, int argc);
int cmd_account_nonce    (Blockchain *bc, char **args, int argc);

/* ── Fraud and audit ─────────────────────────────────────────────────────── */
int cmd_fraud_review        (Blockchain *bc, char **args, int argc);
int cmd_approve_suspicious  (Blockchain *bc, char **args, int argc);
int cmd_reject_suspicious   (Blockchain *bc, char **args, int argc);
int cmd_transaction_history (Blockchain *bc, char **args, int argc);
int cmd_provider_history    (Blockchain *bc, char **args, int argc);
int cmd_premium_history     (Blockchain *bc, char **args, int argc);
int cmd_claim_history       (Blockchain *bc, char **args, int argc);

/* ── Internal helper: parse a line into argv-style tokens ────────────────── */
int cli_tokenise(char *line, char **argv, int max_args);

#endif /* CLI_H */
