#include "db.h"

void execute(connection *C, string cmd) {
    work W(*C);

    try{
        W.exec(cmd);
        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}

void dropTable(connection *C, string fname) {
  string cmd;
  cmd = "DROP TABLE IF EXISTS " + fname + " CASCADE;";
  execute(C, cmd);
}

void createTables(connection *C) {
    createAccount(C);
    createPosition(C);
    createOpenOrder(C);
    createExecOrder(C);
    createCancelOrder(C);
}

void createAccount(connection *C) {
    string cmd = "CREATE TABLE ACCOUNT(ACCOUNT_ID VARCHAR(50) PRIMARY KEY NOT NULL, BALANCE NUMERIC NOT NULL CHECK (BALANCE >= 0));";
    execute(C, cmd);
}

void createPosition(connection *C) {
    string cmd = "CREATE TABLE POSITION(POSITION_ID SERIAL PRIMARY KEY, SYMBOL VARCHAR(50) NOT NULL, AMOUNT NUMERIC NOT NULL CHECK (AMOUNT >= 0), ACCOUNT_ID VARCHAR(50), " \
    "FOREIGN KEY(ACCOUNT_ID) REFERENCES ACCOUNT ON DELETE CASCADE ON UPDATE CASCADE);";
    execute(C, cmd);
}

void createOpenOrder(connection *C) {
    string cmd = "CREATE TABLE OPEN_ORDER(TRAN_ID SERIAL PRIMARY KEY, SYMBOL VARCHAR(50) NOT NULL, AMOUNT NUMERIC NOT NULL, LIMIT_PRICE NUMERIC CHECK (LIMIT_PRICE > 0), ACCOUNT_ID VARCHAR(50), STATUS INT NOT NULL DEFAULT 1, "\
    "FOREIGN KEY(ACCOUNT_ID) REFERENCES ACCOUNT ON DELETE CASCADE ON UPDATE CASCADE);";
    execute(C, cmd);
}

void createExecOrder(connection *C) {
    string cmd = "CREATE TABLE EXEC_ORDER(ORDER_EXEC_ID SERIAL PRIMARY KEY, SYMBOL VARCHAR(50) NOT NULL, AMOUNT NUMERIC NOT NULL, EXEC_PRICE NUMERIC CHECK (EXEC_PRICE > 0), BUYER_ACCOUNT_ID VARCHAR(50), SELLER_ACCOUNT_ID VARCHAR(50), BUYER_TRAN_ID INT, SELLER_TRAN_ID INT, TIME BIGINT, "\
    "FOREIGN KEY(BUYER_ACCOUNT_ID) REFERENCES ACCOUNT ON DELETE CASCADE ON UPDATE CASCADE," \
    "FOREIGN KEY(SELLER_ACCOUNT_ID) REFERENCES ACCOUNT ON DELETE CASCADE ON UPDATE CASCADE);";
    execute(C, cmd);
}

void createCancelOrder(connection *C) {
    string cmd = "CREATE TABLE CANCEL_ORDER(ORDER_CANCEL_ID SERIAL PRIMARY KEY, SYMBOL VARCHAR(50) NOT NULL, AMOUNT NUMERIC NOT NULL, CANCEL_PRICE NUMERIC CHECK (CANCEL_PRICE > 0), ACCOUNT_ID VARCHAR(50), TRAN_ID INT, TIME BIGINT, "\
    "FOREIGN KEY(ACCOUNT_ID) REFERENCES ACCOUNT ON DELETE CASCADE ON UPDATE CASCADE,"\
    "FOREIGN KEY(TRAN_ID) REFERENCES OPEN_ORDER ON DELETE CASCADE ON UPDATE CASCADE);";
    execute(C, cmd);
}

void addAccount(connection *C, string acc_id, string balance) {
    work W(*C);
    stringstream ss;
    ss << "INSERT INTO ACCOUNT(ACCOUNT_ID, BALANCE) VALUES('" << acc_id << "'," << balance << ");"; 
    
    try{
        W.exec(ss.str());
        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}

void addPosition(connection *C, string sym, string amount, string acc_id) {
    work W(*C);
    stringstream sql, insert;
    sql << "SELECT * FROM POSITION WHERE SYMBOL = '" << sym << "' AND ACCOUNT_ID = '" << acc_id << "';";
    result R(W.exec(sql.str()));

    // if is a new symbol
    if (!R.size()) {
        insert << "INSERT INTO POSITION(SYMBOL, AMOUNT, ACCOUNT_ID) VALUES('" << sym << "'," << stod(amount) << ",'" << acc_id << "');";
    }
    // update amount
    else {
        insert << "UPDATE POSITION SET AMOUNT = (AMOUNT + " << stod(amount) << ") WHERE (ACCOUNT_ID='" << acc_id << "' AND SYMBOL='" << sym << "');";
    }

    try{
        W.exec(insert.str());
        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
        
}

int selectNewOpenOrderID(connection *C){
    string cmd = "SELECT * FROM OPEN_ORDER ORDER BY TRAN_ID DESC LIMIT 1;";
    work W(*C);
    result R( W.exec( cmd ));
    result::const_iterator c = R.begin();
    int account_id = c[0].as<int>();
    W.commit();

    return account_id;
}

double checkBalance(connection *C, string sym_amt, string limit_pr, string acc_id) {
    work W(*C);
    double amount = stod(sym_amt);
    double limit = stod(limit_pr);
    double cost = amount*limit;

    string cmd = "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID=\'" + acc_id + "\';";
    result R(W.exec( cmd ));
    result::const_iterator c = R.begin();
    double acc_balance = c[1].as<double>();
    
    W.commit();

    if(acc_balance<cost){
        return -1;
    }
    else{
        return (acc_balance-cost);
    }
}

void addBuyOpenOrder(connection *C, string sym, string sym_amt, string limit_pr, string acc_id, double new_balance) {
    //insert into open_order the order
    work W(*C);

    try{
        stringstream insert_ss;
        insert_ss << "INSERT INTO OPEN_ORDER(SYMBOL,AMOUNT,LIMIT_PRICE,ACCOUNT_ID) VALUES('" << sym << "'," << sym_amt << "," << limit_pr << ",'" << acc_id << "');";
        string cmd = insert_ss.str();
        W.exec(cmd);

        //deduct money from buyer's account
        stringstream update_ss;
        update_ss << "UPDATE ACCOUNT SET BALANCE=" << new_balance << " WHERE ACCOUNT_ID='" << acc_id << "';";
        cmd = update_ss.str();
        W.exec(cmd);

        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}

double checkAmount(connection *C, string sym, string sym_amt, string acc_id) {
    work W(*C);
    stringstream  select_ss;
    select_ss << "SELECT * FROM POSITION WHERE ACCOUNT_ID='" << acc_id << "' AND SYMBOL='" << sym << "';";
    string cmd = select_ss.str();
    result R(W.exec( cmd ));
    W.commit();

    if(R.size()){
        result::const_iterator c = R.begin();
        double old_symbol_amount = c[2].as<double>();
        double symbol_amount = stod(sym_amt);
        cout << "======sym_amt: " << sym_amt << endl; 

        if((old_symbol_amount+symbol_amount)<0){
            return -1;
        }
        else{
            return (old_symbol_amount+symbol_amount);
        }
    }
    
    return -1;   
}

void addSellOpenOrder(connection *C, string sym, string sym_amt, string limit_pr, string acc_id, double new_shares) {
    //insert into open_order the order
    work W(*C);

    try{
        stringstream insert_ss;
        insert_ss << "INSERT INTO OPEN_ORDER(SYMBOL,AMOUNT,LIMIT_PRICE,ACCOUNT_ID) VALUES('" << sym << "'," << sym_amt << "," << limit_pr << ",'" << acc_id << "');";
        W.exec(insert_ss.str());

        //deduct shares from seller's position
        stringstream update_ss;
        update_ss << "UPDATE POSITION SET AMOUNT=" << new_shares << " WHERE ACCOUNT_ID='" << acc_id << "' AND SYMBOL='" << sym << "';";
        cout << "~~~~~~update_ss: " << update_ss.str() << endl;
        W.exec(update_ss.str());

        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}


void addExecOrder(connection *C, string sym, string amount, string exec_price, string seller_id, string buyer_id, int buyer_tran_id, int seller_tran_id, int buyer_flag, int seller_flag) {
    stringstream cmd, update_cmd, update_cmd2, update_cmd3, update_cmd4, update_cmd5, select_cmd;

    time_t t = time(0);

    work W(*C);

    try{
        if (stod(exec_price) > 0) {
            //insert into exec_order table
            cmd << "INSERT INTO EXEC_ORDER(SYMBOL,AMOUNT,EXEC_PRICE,BUYER_ACCOUNT_ID,SELLER_ACCOUNT_ID,BUYER_TRAN_ID,SELLER_TRAN_ID,TIME) VALUES('" << sym << "'," << amount << "," << exec_price << ",'" << buyer_id << "','" << seller_id  << "'," << buyer_tran_id << "," << seller_tran_id << "," << t << ");";
            W.exec(cmd);
            
            //executing
            //adding money to the seller’s account
            update_cmd << "UPDATE ACCOUNT SET BALANCE = (BALANCE + " + to_string(stod(exec_price) * stod(amount)) + ") WHERE ACCOUNT_ID = '" << seller_id << "';";
            W.exec(update_cmd);

            //create new position if needed
            select_cmd <<  "SELECT * FROM POSITION WHERE SYMBOL=\'" << sym << "\' AND ACCOUNT_ID=\'" << buyer_id << "\';";
            result R(W.exec(select_cmd.str()));

            //adjust shares
            if (R.size()) {
                update_cmd2 << "UPDATE POSITION SET AMOUNT = (AMOUNT + " + amount + ") WHERE ACCOUNT_ID ='" << buyer_id << "' AND SYMBOL='" << sym << "';";
                cout << "update_cmd2: " << update_cmd2.str() << endl;
            }
            else {
                update_cmd2 << "INSERT INTO POSITION(SYMBOL,AMOUNT,ACCOUNT_ID) VALUES('" << sym << "'," << amount << ",'" << buyer_id << "');";
            }
            W.exec(update_cmd2);

            //delete from open_order by checking amount & sellerId || buyerID?
            //delete_cmd << "DELETE FROM OPEN_ORDER WHERE AMOUNT=" + amount + " AND (ACCOUNT_ID='" + seller_id + "' OR ACCOUNT_ID='" + buyer_id + "');";
            if(buyer_flag && seller_flag){
                update_cmd4 << "UPDATE OPEN_ORDER SET STATUS = 0 WHERE (TRAN_ID = " << buyer_tran_id << "OR TRAN_ID = "<< seller_tran_id << ");";
            }
            else if (buyer_flag) {
                update_cmd4 << "UPDATE OPEN_ORDER SET STATUS = 0 WHERE (TRAN_ID = " << buyer_tran_id << ");";
                string sql = "UPDATE OPEN_ORDER SET AMOUNT = (AMOUNT+" + amount + ") WHERE TRAN_ID = " + to_string(seller_tran_id) + ";"; 
                cout << "update: " << sql << endl;
                W.exec(sql);
            }
            else {
                update_cmd4 << "UPDATE OPEN_ORDER SET STATUS = 0 WHERE (TRAN_ID = " << seller_tran_id << ");";
                string sql = "UPDATE OPEN_ORDER SET AMOUNT = (AMOUNT-" + amount + ") WHERE TRAN_ID = " + to_string(buyer_tran_id) + ";"; 
                cout << "update: " << sql << endl;
                W.exec(sql);
            }
            W.exec(update_cmd4);

            W.commit();
        }
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}

void refundBuyerAccount(connection *C, double b_price, double exec_price, double amount, int b_acc_id){
    //refund money to the buyer’s account if execution price is lower than buyer's price
    work W(*C);

    try{
        stringstream update_cmd;
        update_cmd << "UPDATE ACCOUNT SET BALANCE = (BALANCE + " <<  (b_price-exec_price)*amount << ") WHERE ACCOUNT_ID = '" << b_acc_id << "';";
        W.exec(update_cmd);
        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
    
}

bool checkCancelStatus(connection *C, string tran_ID){
    work W(*C);
    string cmd = "SELECT * FROM OPEN_ORDER WHERE TRAN_ID = " + tran_ID + "AND STATUS=1;";
    result R(W.exec( cmd ));
    W.commit();

    if(R.size()){
        return true;
    }
    else{
        return false;
    }
}

void addCancelOrder(connection *C, string tran_id) {
    stringstream cmd, delete_cmd, update_cmd1, sql;
    time_t t = time(0);

    work W(*C);
    
    try{
        string sql2 = "SELECT * FROM OPEN_ORDER WHERE TRAN_ID = " + tran_id + ";";
        result R( W.exec(sql2) );
        result::const_iterator c = R.begin();
        string sym = c[1].as<string>();
        double amount = c[2].as<double>();
        double cancel_price = c[3].as<double>();
        string account_id = c[4].as<string>();
        
        cmd << "INSERT INTO CANCEL_ORDER(SYMBOL,AMOUNT,CANCEL_PRICE,ACCOUNT_ID,TRAN_ID,TIME) VALUES('" << sym << "'," << amount << "," << cancel_price << ",'" << account_id << "'," << tran_id << "," << t << ");";
        W.exec(cmd);

        //delete from open order, do refund
        delete_cmd << "UPDATE OPEN_ORDER SET STATUS = 0 WHERE TRAN_ID = " << tran_id << ";";
        W.exec(delete_cmd);

        if (amount < 0) {
            update_cmd1 << "UPDATE POSITION SET AMOUNT = (AMOUNT + " << to_string(abs(amount)) + ") WHERE ACCOUNT_ID='" << account_id << "' AND SYMBOL='" << sym << "';";
        }
        else {
            update_cmd1 << "UPDATE ACCOUNT SET BALANCE = (BALANCE + " + to_string(cancel_price * amount) + ") WHERE ACCOUNT_ID='" << account_id << "';";
        }
        W.exec(update_cmd1);

        W.commit();
    }
    catch(exception & e){
        W.abort();
        cerr << e.what() << endl;
    }
}

double queryOpenOrder(connection *C, string account_id, string tran_ID){
    stringstream query_ss;
    query_ss << "SELECT * FROM OPEN_ORDER WHERE ACCOUNT_ID='" << account_id <<  "' AND TRAN_ID=" << tran_ID <<  " AND STATUS=1;";
    string cmd = query_ss.str();
    double amount = -1;
    
    work W(*C);
    result R(W.exec( cmd ));

    if(R.size()){
        result::const_iterator c = R.begin();
        amount = c[2].as<double>();
    }

    W.commit();
    return amount;
}

pair<double,int> queryCancelOrder(connection *C, string account_id, string tran_ID){
    stringstream query_ss;
    query_ss << "SELECT * FROM CANCEL_ORDER WHERE ACCOUNT_ID='" << account_id << "' AND TRAN_ID=" << tran_ID << ";";
    string cmd = query_ss.str();
    double amount = -1;
    int time = -1;
    
    work W(*C);
    result R(W.exec( cmd ));

    if(R.size()){
        result::const_iterator c = R.begin();
        amount = c[2].as<double>();
        time = c[6].as<int>();
    }

    W.commit();

    pair<double,int> cancel_info(amount, time);
    return cancel_info;
}

vector<exec_info> queryExecOrder(connection *C, string account_id, string tran_ID){
    stringstream query_ss, getID;
    query_ss << "SELECT * FROM EXEC_ORDER WHERE (BUYER_ACCOUNT_ID='" << account_id << "' AND BUYER_TRAN_ID=" << tran_ID << ") OR (SELLER_ACCOUNT_ID='" << account_id << "' AND SELLER_TRAN_ID=" << tran_ID << ");";
    string cmd = query_ss.str();
    vector<exec_info> v_info;
    
    work W(*C);
    result R(W.exec( cmd ));

   
   getID << "SELECT * FROM EXEC_ORDER WHERE (BUYER_ACCOUNT_ID='" << account_id << "' AND BUYER_TRAN_ID=" << tran_ID << ");";
    result R2(W.exec(getID.str()));
    int r;
    if (R2.size()) {
        r = 1;
    }
    else {
        r = -1;
    }

    if(R.size()){
        for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
            exec_info info;
            info.amount = r * c[2].as<double>();
            info.exec_price = c[3].as<double>();
            info.time = c[8].as<int>();
            v_info.push_back(info);
        }
    }

    W.commit();

    return v_info;
}

result getMatching(connection *C) {
    stringstream sql, sql2, sql3, sql4, sql5;
    work W(*C);
    sql << "CREATE VIEW SELL_MATCHING(S_TRAN_ID, S_SYMBOL, S_AMOUNT, S_PRICE, S_ACC_ID) AS SELECT TRAN_ID, SYMBOL, AMOUNT, LIMIT_PRICE, ACCOUNT_ID "
    << "FROM OPEN_ORDER WHERE (STATUS = 1 AND AMOUNT < 0) ORDER BY LIMIT_PRICE;";
    W.exec(sql);

    sql2 << "CREATE VIEW BUY_MATCHING(B_TRAN_ID, B_SYMBOL, B_AMOUNT, B_PRICE, B_ACC_ID) "
    << "AS SELECT TRAN_ID, SYMBOL, AMOUNT, LIMIT_PRICE, ACCOUNT_ID FROM OPEN_ORDER WHERE (STATUS = 1 AND AMOUNT > 0) ORDER BY LIMIT_PRICE DESC;";
    W.exec(sql2);
    
    sql3 << "CREATE VIEW MATCHING AS SELECT S_TRAN_ID, B_TRAN_ID, S_AMOUNT, B_AMOUNT, B_SYMBOL, S_PRICE, "
    << "B_PRICE, B_ACC_ID, S_ACC_ID FROM SELL_MATCHING, BUY_MATCHING WHERE (B_PRICE >= S_PRICE AND B_SYMBOL = S_SYMBOL AND B_ACC_ID != S_ACC_ID);";
    W.exec(sql3);

    sql4 << "SELECT * FROM MATCHING;";
    result R(W.exec(sql4));

    sql5 << "DROP VIEW MATCHING, BUY_MATCHING, SELL_MATCHING;";
    W.exec(sql5);

    W.commit();

    return R;
}

void findMatch(connection *C) {
    result R = getMatching(C);

    if (!R.size()){
        return;
    }

    result::const_iterator c = R.begin();    
    // determine amount
    // if buy amount less & equal to sell amount, set amount to buy
    double amount = -1;
    double seller_amount = abs(c[2].as<double>());
    double buyer_amount = abs(c[3].as<double>());
    if (buyer_amount <= seller_amount) {
        amount = buyer_amount;
    }
    else{
        amount = seller_amount;
    }

    // determine price by checking tran_id
    int seller_tran_id = c[0].as<int>();
    int buyer_tran_id = c[1].as<int>();
    string b_symbol = c[4].as<string>();
    double s_price = c[5].as<double>();
    double b_price = c[6].as<double>();
    int b_acc_id = c[7].as<int>();
    int s_acc_id = c[8].as<int>();
    int buyer_flag = -1;
    int seller_flag = -1;
    double exec_price = -1;

    //set execution price
    if (seller_tran_id > buyer_tran_id) {
        exec_price = b_price;
    }
    else {
        exec_price = s_price;
    }

    //set buyer flag and seller flag
    if(seller_amount==buyer_amount){
        buyer_flag = 1;
        seller_flag = 1;
    }
    else if(seller_amount>buyer_amount) {
        buyer_flag = 1;
        seller_flag = 0;
    }
    else{
        buyer_flag = 0;
        seller_flag = 1;
    }

    addExecOrder(C, b_symbol, to_string(amount), to_string(exec_price), to_string(s_acc_id), to_string(b_acc_id), buyer_tran_id, seller_tran_id, buyer_flag, seller_flag);
    
    //refund to buyer if the execution price is lower
    if(exec_price < b_price) {
        refundBuyerAccount(C, b_price, exec_price, amount, b_acc_id);
        cout << "Refunding..." << endl;
    }

    //check if the amount is 0

    findMatch(C);
}
