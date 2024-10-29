#include <iostream>
#include <sstream>
#include <vector>
#include <random>

#include <mysql/mysql.h>

#define DEFAULT_USERNAME "Default_Username"
#define DEFAULT_TEXT "Default_Text"
#define DEFAULT_PASSWORD "Default_Password"
#define DEFAULT_AUTH_TOKEN "Default_Auth_Token"
#define DEFAULT_REFRESH_TOKEN "Default_Refresh_Token"
#define DEFAULT_RECEIVER_USERNAME "Default_Reciver_Username"
#define DEFAULT_SENDING_DATA "Default_Sending_Data"
#define DEFAULT_CHATTER_USERNAME "Default_Chatter_Username"

class Message {
    private:
        std::string Text;
        std::string Username;
        std::string Receiver_Username;
        std::string Auth_Token;
        std::string Sending_Data; // format "yyyy-mm-dd"

    public:
        Message() {
            this->Text = DEFAULT_TEXT;
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Receiver_Username = DEFAULT_RECEIVER_USERNAME;
            this->Username = DEFAULT_USERNAME;
            this->Sending_Data = DEFAULT_SENDING_DATA;
        }

        Message(std::string Text, std::string Username, std::string Receiver_Username, std::string Auth_Token, std::string Sending_Data) {
            this->Text = Text;
            this->Username = Username;
            this->Receiver_Username = Receiver_Username;
            this->Auth_Token = Auth_Token;
            this->Sending_Data = Sending_Data;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Text(std::string New_Text) { this->Text = New_Text; }
        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Receiver_Username(std::string New_Receiver_Username) { this->Receiver_Username = New_Receiver_Username; }
        void Set_Sending_Data(std::string New_Sending_Data) { this->Sending_Data = New_Sending_Data; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Receiver_Username() { return this->Receiver_Username; }
        std::string Get_Text() { return this->Text; }
        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Sending_Data() { return this->Sending_Data; }

        std::string Serialize() {
            return (this->Username + ";" + this->Receiver_Username + ";" + this->Auth_Token + ";" + this->Text + ";" + this->Sending_Data + ";");
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 5) {
                this->Username = tokens[0];
                this->Receiver_Username = tokens[1];
                this->Auth_Token = tokens[2];
                this->Text = tokens[3];
                this->Sending_Data = tokens[4];
            } else {
                this->Text = DEFAULT_TEXT;
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Receiver_Username = DEFAULT_RECEIVER_USERNAME;
                this->Username = DEFAULT_USERNAME;
                this->Sending_Data = DEFAULT_SENDING_DATA;
            }
        }
};

class Login_Request {
    private:
        std::string Username;
        std::string Password;

    public:
        Login_Request() {
            this->Password = DEFAULT_PASSWORD;
            this->Username = DEFAULT_USERNAME;
        }

        Login_Request(std::string Username, std::string Password) {
            this->Username = Username;
            this->Password = Password;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Password(std::string New_Password) { this->Password = New_Password; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Password() { return this->Password; }

        std::string Serialize() {
            return this->Username + ";" + this->Password + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Username = tokens[0];
                this->Password = tokens[1];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Password = DEFAULT_PASSWORD;
            }
        }
};

class Login_Response {
    private:
        std::string Auth_Token;
        std::string Refresh_Token;

    public:
        Login_Response() {
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
        }

        Login_Response(std::string Auth_Token, std::string Refresh_Token) {
            this->Auth_Token = Auth_Token;
            this->Refresh_Token = Refresh_Token;
        }

        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Refresh_Token(std::string New_Refresh_Token) { this->Refresh_Token = New_Refresh_Token; }

        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Refresh_Token() { return this->Refresh_Token; }

        std::string Serialize() {
            return this->Auth_Token + ";" + this->Refresh_Token + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Auth_Token = tokens[0];
                this->Refresh_Token = tokens[1];
            } else {
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
            }
        }
};

class Register_Request {
    private:
        std::string Username;
        std::string Password;

    public:
        Register_Request() {
            this->Password = DEFAULT_PASSWORD;
            this->Username = DEFAULT_USERNAME;
        }

        Register_Request(std::string Username, std::string Password) {
            this->Username = Username;
            this->Password = Password;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Password(std::string New_Password) { this->Password = New_Password; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Password() { return this->Password; }

        std::string Serialize() {
            return this->Username + ";" + this->Password + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Username = tokens[0];
                this->Password = tokens[1];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Password = DEFAULT_PASSWORD;
            }
        }
};

class Register_Response {
    private:
        std::string Auth_Token;
        std::string Refresh_Token;

    public:
        Register_Response() {
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
        }

        Register_Response(std::string Auth_Token, std::string Refresh_Token) {
            this->Auth_Token = Auth_Token;
            this->Refresh_Token = Refresh_Token;
        }

        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Refresh_Token(std::string New_Refresh_Token) { this->Refresh_Token = New_Refresh_Token; }

        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Refresh_Token() { return this->Refresh_Token; }

        std::string Serialize() {
            return this->Auth_Token + ";" + this->Refresh_Token + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Auth_Token = tokens[0];
                this->Refresh_Token = tokens[1];
            } else {
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
            }
        }
};

class Refresh_Token_Request {
    private:
        std::string Username;
        std::string Refresh_Token;

    public:
        Refresh_Token_Request() {
            this->Username = DEFAULT_USERNAME;
            this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
        }

        Refresh_Token_Request(std::string Username, std::string Refresh_Token) {
            this->Username = Username;
            this->Refresh_Token = Refresh_Token;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Refresh_Token(std::string New_Refresh_Token) { this->Refresh_Token = New_Refresh_Token; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Refresh_Token() { return this->Refresh_Token; }

        std::string Serialize() {
            return this->Username + ";" + this->Refresh_Token + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Username = tokens[0];
                this->Refresh_Token = tokens[1];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
            }
        }
};

class Refresh_Token_Response {
    private:
        std::string Auth_Token;
        std::string Refresh_Token;

    public:
        Refresh_Token_Response() {
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
        }

        Refresh_Token_Response(std::string Auth_Token, std::string Refresh_Token) {
            this->Auth_Token = Auth_Token;
            this->Refresh_Token = Refresh_Token;
        }

        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Refresh_Token(std::string New_Refresh_Token) { this->Refresh_Token = New_Refresh_Token; }

        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Refresh_Token() { return this->Refresh_Token; }

        std::string Serialize() {
            return this->Auth_Token + ";" + this->Refresh_Token + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Auth_Token = tokens[0];
                this->Refresh_Token = tokens[1];
            } else {
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Refresh_Token = DEFAULT_REFRESH_TOKEN;
            }
        }
};

class Get_All_Messages_Request {
    private:
        std::string Username;
        std::string Chatter_Username;
        std::string Auth_Token;

    public:
        Get_All_Messages_Request() {
            this->Username = DEFAULT_USERNAME;
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Chatter_Username = DEFAULT_CHATTER_USERNAME;
        }

        Get_All_Messages_Request(std::string Username, std::string Auth_Token, std::string Chatter_Username) {
            this->Username = Username;
            this->Auth_Token = Auth_Token;
            this->Chatter_Username = Chatter_Username;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Chatter_Username(std::string New_Chatter_Username) { this->Chatter_Username = New_Chatter_Username; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Chatter_Username() { return this->Chatter_Username; }

        std::string Serialize() {
            return this->Username + ";" + this->Auth_Token + ";" + this->Chatter_Username + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 3) {
                this->Username = tokens[0];
                this->Auth_Token = tokens[1];
                this->Chatter_Username = tokens[2];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Chatter_Username = DEFAULT_CHATTER_USERNAME;
            }
        }
};

class Get_All_Messages_Response {
    private:
        std::vector<Message> Message_List;

    public:
        Get_All_Messages_Response() {
            Message_List.clear();
        }

        Get_All_Messages_Response(std::vector<Message> Message_List) {
            this->Message_List = Message_List;
        }

        void Set_Message_List(std::vector<Message> New_Message_List) { this->Message_List = New_Message_List; }

        std::vector<Message> Get_Message_List() { return this->Message_List; }

        std::string Serialize() {
            std::string serialized = std::to_string(Message_List.size()) + ";";
            for (auto& message : Message_List) {
                serialized += message.Serialize();
            }
            return serialized;
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            size_t count = std::stoul(tokens[0]);
            Message_List.clear();

            for (size_t i = 1; i < count + 1; ++i) {
                Message message;
                message.Deserialize(tokens[i]);
                Message_List.push_back(message);
            }
        }
};

class Delete_History_Request {
    private:
        std::string Username;
        std::string Auth_Token;

    public:
        Delete_History_Request() {
            this->Username = DEFAULT_USERNAME;
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
        }

        Delete_History_Request(std::string Username, std::string Auth_Token) {
            this->Username = Username;
            this->Auth_Token = Auth_Token;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Auth_Token() { return this->Auth_Token; }

        std::string Serialize() {
            return this->Username + ";" + this->Auth_Token + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 2) {
                this->Username = tokens[0];
                this->Auth_Token = tokens[1];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
            }
        }
};

class Search_Messages_Request {
    private:
        std::string Username;
        std::string Chatter_Username;
        std::string Auth_Token;

    public:
        Search_Messages_Request() {
            this->Username = DEFAULT_USERNAME;
            this->Auth_Token = DEFAULT_AUTH_TOKEN;
            this->Chatter_Username = DEFAULT_CHATTER_USERNAME;
        }

        Search_Messages_Request(std::string Username, std::string Auth_Token, std::string Chatter_Username) {
            this->Username = Username;
            this->Auth_Token = Auth_Token;
            this->Chatter_Username = Chatter_Username;
        }

        void Set_Username(std::string New_Username) { this->Username = New_Username; }
        void Set_Auth_Token(std::string New_Auth_Token) { this->Auth_Token = New_Auth_Token; }
        void Set_Chatter_Username(std::string New_Chatter_Username) { this->Chatter_Username = New_Chatter_Username; }

        std::string Get_Username() { return this->Username; }
        std::string Get_Auth_Token() { return this->Auth_Token; }
        std::string Get_Chatter_Username() { return this->Chatter_Username; }

        std::string Serialize() {
            return this->Username + ";" + this->Auth_Token + ";" + this->Chatter_Username + ";";
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            if (tokens.size() == 3) {
                this->Username = tokens[0];
                this->Auth_Token = tokens[1];
                this->Chatter_Username = tokens[2];
            } else {
                this->Username = DEFAULT_USERNAME;
                this->Auth_Token = DEFAULT_AUTH_TOKEN;
                this->Chatter_Username = DEFAULT_CHATTER_USERNAME;
            }
        }
};

class Search_Messages_Response {
    private:
        std::vector<Message> Message_List;

    public:
        Search_Messages_Response() {
            Message_List.clear();
        }

        Search_Messages_Response(std::vector<Message> Message_List) {
            this->Message_List = Message_List;
        }

        void Set_Message_List(std::vector<Message> New_Message_List) { this->Message_List = New_Message_List; }

        std::vector<Message> Get_Message_List() { return this->Message_List; }

        std::string Serialize() {
            std::string serialized = std::to_string(Message_List.size()) + ";";
            for (auto& message : Message_List) {
                serialized += message.Serialize();
            }
            return serialized;
        }

        void Deserialize(const std::string& serialized) {
            std::stringstream ss(serialized);
            std::string item;
            std::vector<std::string> tokens;

            while (std::getline(ss, item, ';')) {
                if (!item.empty()) {
                    tokens.push_back(item);
                }
            }

            size_t count = std::stoul(tokens[0]);
            Message_List.clear();

            for (size_t i = 1; i < count + 1; ++i) {
                Message message;
                message.Deserialize(tokens[i]);
                Message_List.push_back(message);
            }
        }
};

class DB_Connection{

    private:
        MYSQL *DB_Conn;
        MYSQL_RES *DB_Res;
        MYSQL_ROW DB_Row;

        const char *DB_Server = "localhost";
        const char *DB_User = "tridoz";
        const char *DB_Password = "Mogg4356%#TRIDAPALI";
        const char *DB_Name = "DB_Capolavoro";

        int _does_token_exist( std::string token){
            std::string Query  = "SELECT * FROM Tokens WHERE Token="+token+";";

            if( mysql_query( this->DB_Conn, Query.c_str() ) ){
                std::cerr<<"chekcing if the token already existed FAILED: "<<mysql_error( this->DB_Conn )<<"\n";
                return -1;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );

            int num_rows = mysql_num_rows( this->DB_Res );

            return num_rows;
        }


    public:
        DB_Connection(){
            DB_Conn = nullptr;
            DB_Res = nullptr;
            DB_Row = nullptr;
        }

        bool Connect(){
            this->DB_Conn = mysql_init(nullptr);
            if(this->DB_Conn == nullptr){
                std::cerr << "Database Connection Failed\n";
                return false;
            }

            if( mysql_real_connect(this->DB_Conn, this->DB_Server, this->DB_User, this->DB_Password, this->DB_Name, 0, nullptr, 0) == nullptr){
                std::cerr << "mysql_real_connect() failed\n";
                mysql_close(DB_Conn);
                return false;
            }

            return true;
        }

        bool check_token_validity(std::string token, std::string Username) {
            std::string Query_For_UserID = "SELECT UserID FROM Users WHERE Username="+Username+"";

            if( mysql_query( this->DB_Conn , Query_For_UserID.c_str() )){
                std::cerr<<"Searchnig the UserName <" << Username << "> for his UserID has FAILED"<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );

            if( this->DB_Res == nullptr ){
                std::cerr << "mysql_store_result() has Failed: " << mysql_error(DB_Conn) << "\n";
                return false;
            }

            this->DB_Row = mysql_fetch_row( this->DB_Res );

            if( this->DB_Row == nullptr ){
                mysql_free_result(this->DB_Res);
                std::cerr << "Nessun utente trovato o errore." << "\n";
                return false;
            }

            int User_ID = std::stoi( this->DB_Row[0] );
            
            std::string Query_Tokens = "SELECT * FROM Tokens WHERE Token='" + token + "' AND UserID = " + std::to_string(User_ID) + " AND VALID = true;";

            if( mysql_query( this->DB_Conn, Query_Tokens.c_str() ) ){
                std::cerr << "Searching if the token <"+token+"> for the username <"+Username+">as failed: "<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );

            if ( this->DB_Res == nullptr) {
                std::cerr << "Errore MySQL: " << mysql_error( this->DB_Conn ) << std::endl;
                return false;
            }

            int num_rows = mysql_num_rows( this->DB_Res);

            mysql_free_result( this->DB_Res );

            return num_rows == 1;
        }

        bool check_user_exist( std::string Username, std::string Password){
            std::string Query = "SELECT * FROM Users WHERE Username="+Username+" AND User_PWD="+Password+";";

            if( mysql_query( this->DB_Conn, Query.c_str() ) ){
                std::cerr<<"Searching for the User <"+Username+"> has FAILED: "<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );

            if( this->DB_Res == nullptr ){
                std::cerr<<"mysql_store_result() has FAILED: "<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            int num_rows = mysql_num_rows( this->DB_Res);

            return num_rows == 1;
        }

        bool check_username_valid( std::string username ){
            std::string Query = "SELECT * FROM Users WHERE Username="+username+";";

            if( mysql_query( this->DB_Conn, Query.c_str() ) ){
                std::cerr<<"searching if the username <"+username+"> already existed has FAILED"<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );
            int num_rows = mysql_num_rows( this->DB_Res );

            return num_rows == 1;
        }

        std::vector<std::string> generate_tokens(){
            std::vector<std::string> tokens;
            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<> distr(0, 15);

            do
            {

                tokens.clear();
                std::string Auth_Token;
                std::string Refresh_Token;

                for(int i = 0 ; i<32 ; i++){
                    Auth_Token += "0123456789abcdef"[distr(generator)];
                }

                for(int i = 0 ; i<32 ; i++){
                    Refresh_Token += "0123456789abcdef"[distr(generator)];
                }

                tokens.push_back( Auth_Token );
                tokens.push_back( Refresh_Token );

            } while ( _does_token_exist( tokens[0]) == 0 && _does_token_exist( tokens[1]) == 0);

            return tokens;
        }

        bool create_user( std::string Username, std::string Password){
            std::string Query = "INSERTO INTO Users (Username, User_PWD) VALUES ("+Username+", "+Password+");";

            if( mysql_query( this->DB_Conn, Query.c_str() ) < 0 ){
                std::cerr<<"inserting new user into the database has failed"<<mysql_error( this->DB_Conn )<<"\n";
                return false;
            }

            return true;
        }

        std::vector<std::string> get_all_user_messages( std::string Username ){
            std::vector<std::string> messages;

            std::string Query_For_UserID = "SELECT UserID FROM Users WHERE Username="+Username+"";

            if( mysql_query( this->DB_Conn , Query_For_UserID.c_str() )){
                std::cerr<<"Searchnig the UserName <" << Username << "> for his UserID has FAILED"<<mysql_error( this->DB_Conn )<<"\n";
                messages.clear();
                return messages;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );

            if( this->DB_Res == nullptr ){
                std::cerr << "mysql_store_result() has Failed: " << mysql_error(DB_Conn) << "\n";
                messages.clear();
                return messages;
            }

            this->DB_Row = mysql_fetch_row( this->DB_Res );

            if( this->DB_Row == nullptr ){
                mysql_free_result(this->DB_Res);
                std::cerr << "Nessun utente trovato o errore." << "\n";
                messages.clear();
                return messages;
            }

            int User_ID = std::stoi( this->DB_Row[0] );

            std::string Query_for_User_Messages = "SELECT * FROM Messages WHERE SenderUserID=" + std::to_string(User_ID) + " OR ReceiverUserID=" + std::to_string(User_ID) + ";"  ;

            if( mysql_query( this->DB_Conn, Query_for_User_Messages.c_str() ) ){
                std::cerr<<"searching all the message for the user has FAILED: "<<"\n";
                messages.clear();
                return messages;
            }

            this->DB_Res = mysql_store_result( this->DB_Conn );
            if( this->DB_Res == nullptr ){
                std::cerr<<"mysql_sotre_result() has FAILED"<<"\n";
                messages.clear();
                return messages;
            }


            while( ( this->DB_Row =  mysql_fetch_row( this->DB_Res ) ) ){
                //Message(std::string Text, std::string Username, std::string Receiver_Username, std::string Auth_Token, std::string Sending_Data) {
                std::string Query_for_ReceiverUsername = "SELECT Username FROM Users WHERE UserID=" +  std::to_string( atoi( DB_Row[2] ) )  + ";";
                if( mysql_query( this->DB_Conn, Query_for_ReceiverUsername.c_str() ) ){
                    std::cerr<<"errore nella ricerca dello username dal ReceiverUserID"<< mysql_error( this->DB_Conn )<<"\n";
                    messages.clear();
                    return;
                }

                
            }
        }
};

