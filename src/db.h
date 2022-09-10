#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <fstream>
#include <string.h>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;
using namespace pqxx;

struct exec_info{
    double amount;
    double exec_price;
    int time;
};

void execute(connection *C, string cmd);

void dropTable(connection *C, string fname);

void createTables(connection *C);

void createAccount(connection *C);

void createPosition(connection *C);

void createOpenOrder(connection *C);

void createExecOrder(connection *C);

void createCancelOrder(connection *C);

void addAccount(connection *C, string acc_id, string balance);

void addPosition(connection *C, string sym, string amount, string acc_id);

int selectNewOpenOrderID(connection *C);

double checkBalance(connection *C, string sym_amt, string limit_pr, string acc_id);

void addBuyOpenOrder(connection *C, string sym, string sym_amt, string limit_pr, string acc_id, double new_balance);

double checkAmount(connection *C, string sym, string sym_amt, string acc_id);

void addSellOpenOrder(connection *C, string sym, string sym_amt, string limit_pr, string acc_id, double new_shares);

void addExecOrder(connection *C, string sym, string amount, string exec_price, string seller_id, string buyer_id, int buyer_tran_id, int seller_tran_id, int buyer_flag, int seller_flag);

void refundBuyerAccount(connection *C, double b_price, double exec_price, double amount, int b_acc_id);

bool checkCancelStatus(connection *C, string tran_ID);

void addCancelOrder(connection *C, string tran_id);

double queryOpenOrder(connection *C, string account_id, string tran_ID);

pair<double,int> queryCancelOrder(connection *C, string account_id, string tran_ID);

vector<exec_info> queryExecOrder(connection *C, string account_id, string tran_ID);

result getMatching(connection *C);

void findMatch(connection *C);