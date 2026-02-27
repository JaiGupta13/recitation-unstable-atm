#ifndef CATCH_CONFIG_MAIN
#  define CATCH_CONFIG_MAIN
#endif

#include <cmath>
#include <fstream>
#include <sstream>

#include "atm.hpp"
#include "catch.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////
//                             Helper Definitions //
/////////////////////////////////////////////////////////////////////////////////////////////

bool CompareFiles(const std::string& p1, const std::string& p2) {
  std::ifstream f1(p1);
  std::ifstream f2(p2);

  if (f1.fail() || f2.fail()) {
    return false;  // file problem.
  }

  std::string f1_read;
  std::string f2_read;
  while (f1.good() || f2.good()) {
    f1 >> f1_read;
    f2 >> f2_read;
    if (f1_read != f2_read || (f1.good() && !f2.good()) ||
        (!f1.good() && f2.good()))
      return false;
  }
  return true;
}

std::string ReadFileContents(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Test Cases
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Example: Create a new account", "[ex-1]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);
  auto accounts = atm.GetAccounts();
  REQUIRE(accounts.contains({12345678, 1234}));
  REQUIRE(accounts.size() == 1);

  Account sam_account = accounts[{12345678, 1234}];
  REQUIRE(sam_account.owner_name == "Sam Sepiol");
  REQUIRE(sam_account.balance == 300.30);

  auto transactions = atm.GetTransactions();
  REQUIRE(accounts.contains({12345678, 1234}));
  REQUIRE(accounts.size() == 1);
  std::vector<std::string> empty;
  REQUIRE(transactions[{12345678, 1234}] == empty);
}

TEST_CASE("Example: Simple widthdraw", "[ex-2]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);
  atm.WithdrawCash(12345678, 1234, 20);
  auto accounts = atm.GetAccounts();
  Account sam_account = accounts[{12345678, 1234}];

  REQUIRE(sam_account.balance == 280.30);
}

TEST_CASE("Example: Print Prompt Ledger", "[ex-3]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);
  auto& transactions = atm.GetTransactions();
  transactions[{12345678, 1234}].push_back(
      "Withdrawal - Amount: $200.40, Updated Balance: $99.90");
  transactions[{12345678, 1234}].push_back(
      "Deposit - Amount: $40000.00, Updated Balance: $40099.90");
  transactions[{12345678, 1234}].push_back(
      "Deposit - Amount: $32000.00, Updated Balance: $72099.90");
  atm.PrintLedger("./prompt.txt", 12345678, 1234);
  REQUIRE(CompareFiles("./ex-1.txt", "./prompt.txt"));
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Adversarial Test Cases - Expose vulnerabilities in ATM implementation
/////////////////////////////////////////////////////////////////////////////////////////////

// --- CheckBalance (reference - no bugs) ---
TEST_CASE("CheckBalance: Valid account returns correct balance",
          "[CheckBalance]") {
  Atm atm;
  atm.RegisterAccount(11111111, 2222, "Alice", 100.50);
  REQUIRE(atm.CheckBalance(11111111, 2222) == 100.50);
}

TEST_CASE("CheckBalance: Non-existent account throws", "[CheckBalance]") {
  Atm atm;
  atm.RegisterAccount(11111111, 2222, "Alice", 100.0);
  REQUIRE_THROWS_AS(atm.CheckBalance(99999999, 9999), std::invalid_argument);
}

// --- RegisterAccount ---
TEST_CASE("RegisterAccount: Duplicate card_num and pin throws invalid_argument",
          "[RegisterAccount]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Bob", 500.0);
  REQUIRE_THROWS_AS(atm.RegisterAccount(12345678, 1234, "Bob Clone", 100.0),
                    std::invalid_argument);
  // Verify original account unchanged
  REQUIRE(atm.CheckBalance(12345678, 1234) == 500.0);
}

TEST_CASE(
    "RegisterAccount: Same card_num different pin creates separate accounts",
    "[RegisterAccount]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1111, "User1", 100.0);
  atm.RegisterAccount(12345678, 2222, "User2", 200.0);
  REQUIRE(atm.CheckBalance(12345678, 1111) == 100.0);
  REQUIRE(atm.CheckBalance(12345678, 2222) == 200.0);
  REQUIRE(atm.GetAccounts().size() == 2);
}

TEST_CASE(
    "RegisterAccount: Same pin different card_num creates separate accounts",
    "[RegisterAccount]") {
  Atm atm;
  atm.RegisterAccount(11111111, 1234, "UserA", 50.0);
  atm.RegisterAccount(22222222, 1234, "UserB", 75.0);
  REQUIRE(atm.CheckBalance(11111111, 1234) == 50.0);
  REQUIRE(atm.CheckBalance(22222222, 1234) == 75.0);
}

TEST_CASE("RegisterAccount: Zero balance allowed", "[RegisterAccount]") {
  Atm atm;
  atm.RegisterAccount(33333333, 3333, "Zero Balance", 0.0);
  REQUIRE(atm.CheckBalance(33333333, 3333) == 0.0);
}

TEST_CASE("RegisterAccount: Creates transactions entry", "[RegisterAccount]") {
  Atm atm;
  atm.RegisterAccount(44444444, 4444, "Test", 100.0);
  auto& transactions = atm.GetTransactions();
  REQUIRE(transactions.contains({44444444, 4444}));
  REQUIRE(transactions[{44444444, 4444}].empty());
}

TEST_CASE("RegisterAccount: Key order (card_num, pin) not (pin, card_num)",
          "[RegisterAccount]") {
  Atm atm;
  // If implementation uses (pin, card_num) as key, these would collide
  atm.RegisterAccount(1234, 5678, "First", 100.0);
  atm.RegisterAccount(5678, 1234, "Second", 200.0);
  REQUIRE(atm.CheckBalance(1234, 5678) == 100.0);
  REQUIRE(atm.CheckBalance(5678, 1234) == 200.0);
}

// --- WithdrawCash ---
TEST_CASE("WithdrawCash: Negative amount throws invalid_argument",
          "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555555, 5555, "WithdrawTest", 100.0);
  REQUIRE_THROWS_AS(atm.WithdrawCash(55555555, 5555, -10.0),
                    std::invalid_argument);
  REQUIRE(atm.CheckBalance(55555555, 5555) == 100.0);
}

TEST_CASE("WithdrawCash: Zero amount - spec says negative throws, zero valid",
          "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555556, 5556, "ZeroWithdraw", 100.0);
  atm.WithdrawCash(55555556, 5556, 0.0);
  REQUIRE(atm.CheckBalance(55555556, 5556) == 100.0);
}

TEST_CASE("WithdrawCash: Amount exceeding balance throws runtime_error",
          "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555557, 5557, "OverdrawTest", 50.0);
  REQUIRE_THROWS_AS(atm.WithdrawCash(55555557, 5557, 100.0),
                    std::runtime_error);
  REQUIRE(atm.CheckBalance(55555557, 5557) == 50.0);
}

TEST_CASE("WithdrawCash: Withdrawing exact balance succeeds",
          "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555558, 5558, "ExactWithdraw", 75.25);
  atm.WithdrawCash(55555558, 5558, 75.25);
  REQUIRE(atm.CheckBalance(55555558, 5558) == 0.0);
}

TEST_CASE("WithdrawCash: Non-existent account throws", "[WithdrawCash]") {
  Atm atm;
  REQUIRE_THROWS_AS(atm.WithdrawCash(99999999, 9999, 10.0),
                    std::invalid_argument);
}

TEST_CASE("WithdrawCash: Records transaction in ledger", "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555559, 5559, "LedgerTest", 100.0);
  atm.WithdrawCash(55555559, 5559, 25.50);
  auto& transactions = atm.GetTransactions();
  REQUIRE(transactions[{55555559, 5559}].size() == 1);
  REQUIRE(transactions[{55555559, 5559}][0].find("25.50") != std::string::npos);
  REQUIRE(transactions[{55555559, 5559}][0].find("74.50") != std::string::npos);
}

TEST_CASE("WithdrawCash: Balance would go negative - use strict comparison",
          "[WithdrawCash]") {
  Atm atm;
  atm.RegisterAccount(55555560, 5560, "StrictTest", 99.99);
  // Withdrawing 100.00 would make balance -0.01
  REQUIRE_THROWS_AS(atm.WithdrawCash(55555560, 5560, 100.00),
                    std::runtime_error);
  REQUIRE(atm.CheckBalance(55555560, 5560) == 99.99);
}

// --- DepositCash ---
TEST_CASE("DepositCash: Negative amount throws invalid_argument",
          "[DepositCash]") {
  Atm atm;
  atm.RegisterAccount(66666666, 6666, "DepositTest", 100.0);
  REQUIRE_THROWS_AS(atm.DepositCash(66666666, 6666, -50.0),
                    std::invalid_argument);
  REQUIRE(atm.CheckBalance(66666666, 6666) == 100.0);
}

TEST_CASE("DepositCash: Zero amount allowed", "[DepositCash]") {
  Atm atm;
  atm.RegisterAccount(66666667, 6667, "ZeroDeposit", 100.0);
  atm.DepositCash(66666667, 6667, 0.0);
  REQUIRE(atm.CheckBalance(66666667, 6667) == 100.0);
}

TEST_CASE("DepositCash: Non-existent account throws", "[DepositCash]") {
  Atm atm;
  REQUIRE_THROWS_AS(atm.DepositCash(99999999, 9999, 50.0),
                    std::invalid_argument);
}

TEST_CASE("DepositCash: Adds to balance correctly", "[DepositCash]") {
  Atm atm;
  atm.RegisterAccount(66666668, 6668, "AddTest", 100.0);
  atm.DepositCash(66666668, 6668, 25.75);
  REQUIRE(atm.CheckBalance(66666668, 6668) == 125.75);
}

TEST_CASE("DepositCash: Records transaction in ledger", "[DepositCash]") {
  Atm atm;
  atm.RegisterAccount(66666669, 6669, "DepositLedger", 50.0);
  atm.DepositCash(66666669, 6669, 30.25);
  auto& transactions = atm.GetTransactions();
  REQUIRE(transactions[{66666669, 6669}].size() == 1);
  REQUIRE(transactions[{66666669, 6669}][0].find("30.25") != std::string::npos);
  REQUIRE(transactions[{66666669, 6669}][0].find("80.25") != std::string::npos);
}

// --- PrintLedger ---
TEST_CASE("PrintLedger: Non-existent account throws", "[PrintLedger]") {
  Atm atm;
  REQUIRE_THROWS_AS(atm.PrintLedger("./ledger.txt", 99999999, 9999),
                    std::invalid_argument);
}

TEST_CASE("PrintLedger: New account with no transactions prints header only",
          "[PrintLedger]") {
  Atm atm;
  atm.RegisterAccount(77777777, 7777, "Empty Ledger User", 100.0);
  atm.PrintLedger("./empty_ledger.txt", 77777777, 7777);
  std::string content = ReadFileContents("./empty_ledger.txt");
  REQUIRE(content.find("Empty Ledger User") != std::string::npos);
  REQUIRE(content.find("77777777") != std::string::npos);
  REQUIRE(content.find("7777") != std::string::npos);
  REQUIRE(content.find("----------------------------") != std::string::npos);
}

TEST_CASE("PrintLedger: Includes all transactions in order", "[PrintLedger]") {
  Atm atm;
  atm.RegisterAccount(77777778, 7778, "Multi Trans", 100.0);
  atm.WithdrawCash(77777778, 7778, 20.0);
  atm.DepositCash(77777778, 7778, 50.0);
  atm.WithdrawCash(77777778, 7778, 10.0);
  atm.PrintLedger("./multi_ledger.txt", 77777778, 7778);
  std::string content = ReadFileContents("./multi_ledger.txt");
  REQUIRE(content.find("Multi Trans") != std::string::npos);
  REQUIRE(content.find("Withdrawal") != std::string::npos);
  REQUIRE(content.find("Deposit") != std::string::npos);
}

TEST_CASE("PrintLedger: Path traversal - relative path with parent dir",
          "[PrintLedger]") {
  Atm atm;
  atm.RegisterAccount(77777779, 7779, "Path Test", 100.0);
  atm.WithdrawCash(77777779, 7779, 10.0);
  std::string ledger_path = "./bin/ledger_output.txt";
  atm.PrintLedger(ledger_path, 77777779, 7779);
  std::string content = ReadFileContents(ledger_path);
  REQUIRE(content.find("Path Test") != std::string::npos);
}

TEST_CASE("PrintLedger: Correct format with Name, Card Number, PIN",
          "[PrintLedger]") {
  Atm atm;
  atm.RegisterAccount(77777780, 7780, "Format Check", 500.0);
  atm.PrintLedger("./format_ledger.txt", 77777780, 7780);
  std::string content = ReadFileContents("./format_ledger.txt");
  REQUIRE(content.find("Name:") != std::string::npos);
  REQUIRE(content.find("Format Check") != std::string::npos);
  REQUIRE(content.find("Card Number:") != std::string::npos);
  REQUIRE(content.find("77777780") != std::string::npos);
  REQUIRE(content.find("PIN:") != std::string::npos);
  REQUIRE(content.find("7780") != std::string::npos);
}
