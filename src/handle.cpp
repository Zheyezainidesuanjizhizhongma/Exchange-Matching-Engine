#include "handle.h"
#include "db.h"

//check if such account exists
bool checkID(connection *C, string acc_id) {
    work W(*C);
    stringstream sql;
    
    sql << "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID = '" << acc_id << "';";
    result R(W.exec(sql.str()));
    W.commit();

    if (R.size()) return true;
    return false;
}

//check if such transaction_id exists
bool checkTranID(connection *C, string tran_id) {
    work W(*C);
    stringstream sql;
    
    sql << "SELECT * FROM OPEN_ORDER WHERE TRAN_ID = '" << tran_id << "';";
    result R(W.exec(sql.str()));
    W.commit();

    if (R.size()) return true;
    return false;
}

string handle_account(connection *C, XMLElement *tranElem){
    cout << "account=============" << endl;
    stringstream response_ss;
    string acc_id = tranElem->Attribute("id");
    string balance_s = tranElem->Attribute("balance");

    if (checkID(C, acc_id)) {
        response_ss << "<error id=\"" << acc_id << "\">Account already exists</error>\n";
    }
    else {
        double balance = stod(balance_s);

        if(balance>0){
            addAccount(C, acc_id, balance_s);
            response_ss << "<created id=\"" << acc_id << "\"/>\n";
        }
        else {
            response_ss << "<error id=\"" << acc_id << "\">Invalid Balance!</error>\n";
        }
    }

    return response_ss.str();
}

string handle_symbol(connection *C, XMLElement *symElem) {
    cout << "symbol=============" << endl;
    stringstream response_ss;
    string sym = symElem->Attribute("sym");

    //handle child requests
    XMLElement * accElem = symElem->FirstChildElement();
    while (accElem)
    {
        string acc_name = accElem->Value();

        if(acc_name.compare("account")==0){
            string acc_id = accElem->Attribute("id");

            //check if account exists
            if(checkID(C, acc_id)){
                //check if amount is negative
                string amount = accElem->GetText();

                if (stod(amount) <= 0) {
                    response_ss << "<error sym=\"" << sym << "\" id=\"" << acc_id << "\">Invalid Amount!</error>\n";
                }
                else{
                    addPosition(C, sym, amount, acc_id);
                    response_ss << "<created sym=\"" << sym << "\" id=\"" << acc_id << "\"/>\n";
                }
            }
            else{
                response_ss << "<error sym=\"" << sym << "\" id=\"" << acc_id << "\">Invalid Account!</error>\n";
            }
        }
        else{
            //error: the transaction action is invalid: please choose from order,query,cancel
            response_ss << "<error>Invalid symbol action: please write account</error>\n";
        }

        accElem = accElem->NextSiblingElement();
    }

    return response_ss.str();
}

string handle_order(connection *C, XMLElement *tranElem, string account_id) {
    cout << "order============" << endl;
    stringstream response_ss; 
    string symbol = tranElem->Attribute("sym");
    string amount = tranElem->Attribute("amount");
    string limit = tranElem->Attribute("limit");
    bool checkID_flag = checkID(C, account_id); 

    if(checkID_flag){
        if(stod(amount)>0){  //buy order
            //check if the account has sufficient balance
            double new_balance = checkBalance(C, amount, limit, account_id);
            if(new_balance!=-1){
                addBuyOpenOrder(C, symbol, amount, limit, account_id, new_balance);
                int tran_ID = selectNewOpenOrderID(C);
                response_ss << "<opened sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\" id=\"" << tran_ID << "\" />\n";
                findMatch(C);
            }
            else{
                response_ss << "<error sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\">" << "Buyer has insufficient balance" << "</error>\n";
            }
        }
        else if(stod(amount)<0){  //sell order
            //check if the position has sufficient shares
            double new_shares = checkAmount(C, symbol, amount, account_id);
            if(new_shares!=-1){
                addSellOpenOrder(C, symbol, amount, limit, account_id, new_shares);
                int tran_ID = selectNewOpenOrderID(C);
                response_ss << "<opened sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\" id=\"" << tran_ID << "\" />\n";
                findMatch(C);
            }
            else{
                response_ss << "<error sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\">" << "Seller has insufficient shares" << "</error>\n";
            }
        }
        else{
            response_ss << "<error sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\">" << "Open order amount should not be zero" << "</error>\n";
        }
    }
    else{
        response_ss << "<error sym=\"" << symbol << "\" amount=\"" << amount << "\" limit=\"" << limit << "\">" << "Invalid account ID" << "</error>\n"; 
    }

    return response_ss.str();
}

string handle_query(connection *C, XMLElement *tranElem, string account_id) {
    cout << "query============" << endl;
    bool checkID_flag = checkID(C, account_id);
    stringstream ss;

    if(checkID_flag){
        string tran_ID = tranElem->Attribute("id");
        bool checkTranID_flag = checkTranID(C, tran_ID);

        if(checkTranID_flag){
            //open
            double open_shares = queryOpenOrder(C, account_id, tran_ID);

            //canceled
            pair<double,int> cancel_info = queryCancelOrder(C, account_id, tran_ID);
            double cancel_shares = cancel_info.first;
            int cancel_time = cancel_info.second;

            //executed
            vector<exec_info> v_info = queryExecOrder(C, account_id, tran_ID);
            
            if(open_shares==-1 && cancel_shares==-1 && v_info.size()==0){
                ss << "<error>" << "Account ID has invalid transaction ID" << "</error>\n";
            }
            else{
                ss << "<status id=\"" << tran_ID << "\">\n";

                if(open_shares!=-1){
                    ss << "<open shares=\"" << open_shares << "\"/>\n";
                }
                
                if(cancel_shares!=-1){
                    ss << "<canceled shares=\"" << cancel_shares << "\" time=\"" << cancel_time << "\"/>\n";
                }

                if(v_info.size()){
                    for (vector<exec_info>::iterator it = v_info.begin() ; it != v_info.end(); ++it){
                        exec_info info = *it;
                        double exec_shares = info.amount;
                        double price = info.exec_price;
                        int exec_time = info.time;
                        ss << "<executed shares=\"" << exec_shares << "\" price=\"" << price << "\" time=\"" << exec_time << "\"/>\n";
                    }
                }

                ss << "</status>\n";
            }
        }
        else{
            ss << "<error>" << "Invalid transaction ID" << "</error>\n"; 
        }
    }
    else{
        ss << "<error>" << "Invalid account ID" << "</error>\n"; 
    }
    
    return ss.str();
}

string handle_cancel(connection *C, XMLElement *tranElem, string account_id) {
    cout << "cancel============" << endl;
    bool checkID_flag = checkID(C, account_id);
    stringstream ss;

    if(checkID_flag){
        string tran_ID = tranElem->Attribute("id");
        bool checkTranID_flag = checkTranID(C, tran_ID);

        if(checkTranID_flag){
            if(checkCancelStatus(C, tran_ID)){
                addCancelOrder(C, tran_ID);
                ss << "<canceled id=\"" << tran_ID << "\">\n";

                //canceled
                pair<double,int> cancel_info = queryCancelOrder(C, account_id, tran_ID);
                double shares = cancel_info.first;
                int time = cancel_info.second;
                if(shares!=-1){
                    ss << "<canceled shares=\"" << shares << "\" time=\"" << time << "\"/>\n";
                }

                //executed
                vector<exec_info> v_info = queryExecOrder(C, account_id, tran_ID);
                if(v_info.size()){
                    for (vector<exec_info>::iterator it = v_info.begin() ; it != v_info.end(); ++it){
                        exec_info info = *it;
                        double shares = info.amount;
                        double price = info.exec_price;
                        int time = info.time;
                        ss << "<executed shares=\"" << shares << "\" price=\"" << price << "\" time=\"" << time << "\"/>\n";
                    }
                }
            }
            else{
                ss << "<error>" << "No cancel shares available" << "</error>\n"; 
            }
        }
        else{
            ss << "<error>" << "Invalid transaction ID" << "</error>\n"; 
        }
    }
    else{
        ss << "<error>" << "Invalid account ID" << "</error>\n"; 
    }

    return ss.str();
}


string handle_create(connection *C, XMLElement *root) {
    cout << "create===========" << endl;
    stringstream response_ss;
    response_ss << "<results>\n";

    //handle child requests
    XMLElement * tranElem = root->FirstChildElement();
    while (tranElem)
    {
        string tran_name = tranElem->Value();

        if(tran_name.compare("account")==0){
            string account_s = handle_account(C, tranElem);
            response_ss << account_s;
        }
        else if(tran_name.compare("symbol")==0){
            string symbol_s = handle_symbol(C, tranElem);
            response_ss << symbol_s;
        }
        else{
            //error: the transaction action is invalid: please choose from order,query,cancel
            response_ss << "<error>Invalid create action: please choose from account, symbol</error>";
        }

        tranElem = tranElem->NextSiblingElement();
    }
    
    response_ss << "</results>";
    return response_ss.str();
}


string handle_transactions(connection* C, XMLElement* root) {
    cout << "transactions============" << endl;
    const char * account_id = root->Attribute("id");
    stringstream response_ss;
    response_ss << "<results>\n";

    //handle child requests
    XMLElement * tranElem = root->FirstChildElement();
    while (tranElem)
    {
        string tran_name = tranElem->Value();

        if(tran_name.compare("order")==0){
            string order_s = handle_order(C, tranElem, account_id);
            response_ss << order_s;
        }
        else if(tran_name.compare("query")==0){
            string query_s = handle_query(C, tranElem, account_id);
            response_ss << query_s;
        }
        else if(tran_name.compare("cancel")==0){
            string cancel_s = handle_cancel(C, tranElem, account_id);
            response_ss << cancel_s;
        }
        else{
            //error: the transaction action is invalid: please choose from order,query,cancel
            response_ss << "<error>Invalid transaction action: please choose from order,query,cancel</error>";
        }

        tranElem = tranElem->NextSiblingElement();
    }
    
    response_ss << "</results>";
    return response_ss.str();
}

void handle_XML(connection *C, int client_connection_fd) {
    int xml_length = 0;
    char text = '\0';
    vector<char> len_text;
    int ret_recv = -1;

    while((ret_recv=recv(client_connection_fd, &text, 1, 0))>0) {
        len_text.push_back(text);

        if(text=='\n'){
            string len_text_s(len_text.begin(), len_text.end()-1);
            xml_length = atoi(len_text_s.c_str());
            break;
        }

        text = '\0';
    }
    
    char xml_text[xml_length+1];
    recv(client_connection_fd, &xml_text, xml_length+1, 0);
    cout << "xml_length: " << xml_length << endl;
    cout << "xml_text: " << xml_text << endl;
    
    XMLDocument docXml;

    XMLError errXml = docXml.Parse(xml_text);
    string XML_response = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
  
    if(XML_SUCCESS == errXml){
        XMLElement* root = docXml.RootElement();
        
        if(root){
            string root_name = root->Value();

            if(root_name.compare("create")==0){
            XML_response += handle_create(C, root);
            }
            else if(root_name.compare("transactions")==0){
            XML_response += handle_transactions(C, root);
            }
            else{
            //error: root name not exists
            XML_response += "<results><error>Root name does not exist</error></results>";
            }
        }
        else{
            //error: no root Node
            XML_response += "<results><error>No root node found</error></results>";
        }
    }
    else{
        //error: parse xml unsuccessful
        XML_response += "<results><error>Parse XML unsuccessful</error></results>";
    }

    //cout << "XML_response: " << XML_response << endl;
    int response_length = XML_response.size();
    vector <char> response_xml;
    response_xml.resize(response_length);
    response_xml.assign(XML_response.begin(),XML_response.end());

    //send the length of xml
    cout << XML_response << endl;
    //send(client_connection_fd, &response_length, sizeof(int), 0);

    //send the text of xml
    send(client_connection_fd, &(response_xml[0]), response_length, 0);

    close(client_connection_fd);
}
