#include <iostream>
#include <pqxx/pqxx>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include "tinyxml2.h"

using namespace std;
using namespace pqxx;
using namespace tinyxml2;

bool checkID(connection *C, string acc_id);

bool checkTranID(connection *C, string tran_id);

string handle_account(connection *C, XMLElement *tranElem);

string handle_symbol(connection *C, XMLElement *tranElem);

string handle_order(connection *C, XMLElement *tranElem, string account_id);

string handle_query(connection *C, XMLElement *tranElem, string account_id);

string handle_cancel(connection *C, XMLElement *tranElem, string account_id);

string handle_create(connection *C, XMLElement *root);

string handle_transactions(connection* C, XMLElement* root);

void handle_XML(connection *C, int socket_fd);