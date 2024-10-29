#include <iostream>
#include <thread>


#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include "classes.h"

#define MAX_THREADS 20
#define DEFAULT_SERVER_PORT 2000
#define DEFAULT_THREAD_PORT 3000
#define DEFAULT_PORT_SENDING_BUF_LENGTH 80

#define THREAD_PORT_SENDING_BUF_LENGTH 4096
#define THREAD_PORT_RECEIVING_BUF_LENGTH 4096

class Server{
    private:
        
        int Number_Active_Threads;
        std::thread SRV_Threads[MAX_THREADS];
        int Thread_Permits[MAX_THREADS];
        int Default_Port_Listening_Socket;
        int Default_Port_Accept_Socket;
        uint Default_Port_Remote_Length;
        char Default_Port_Snd_Buf[DEFAULT_PORT_SENDING_BUF_LENGTH];       
        struct sockaddr_in Default_Port_Local, Default_Port_Remote;

        void Client_Game(int Thread_ID){
            DB_Connection Thread_DB_Connection;

            if( !Thread_DB_Connection.Connect() ){
                this->Thread_Permits[Thread_ID] = 0;    
                this->Number_Active_Threads--;
                return;
            }

            int Thread_Listening_Socket;
            int Thread_Accepting_Socket;
            uint Thread_Remote_Length;

            char Thread_Sending_Buf[THREAD_PORT_SENDING_BUF_LENGTH];
            char Thread_Receiveng_buf[THREAD_PORT_RECEIVING_BUF_LENGTH];
            int Thread_Received_Length;
            
            struct sockaddr_in Thread_Local, Thread_Remote;
            int Client_Port =  3000 + Thread_ID;

            Thread_Listening_Socket = socket(AF_INET, SOCK_STREAM, 0);

            if(Thread_Listening_Socket < 0){
                this->Thread_Permits[Thread_ID] = 0;
                this->Number_Active_Threads--;
                return;
            }

            Thread_Local.sin_family = AF_INET;
            Thread_Local.sin_port = htons(Client_Port);
            Thread_Local.sin_addr.s_addr = htonl(INADDR_ANY);

            if( bind( Thread_Listening_Socket, (struct sockaddr*)&Thread_Local, sizeof(Thread_Local)) < 0){
                this->Thread_Permits[Thread_ID] = 0;
                this->Number_Active_Threads--;
                close(Thread_Listening_Socket);
                return;
            }

            listen( Thread_Listening_Socket, 1);

            Thread_Remote_Length = sizeof(Thread_Remote);
            Thread_Accepting_Socket = accept(Thread_Listening_Socket, (struct sockaddr*)&Thread_Remote, &Thread_Remote_Length);

            if(Thread_Accepting_Socket < 0){
                this->Thread_Permits[Thread_ID] = 0;
                this->Number_Active_Threads--;
                close(Thread_Listening_Socket);
                return;
            }

            Message User_MSG;
            Message SRV_MSG;

            while( User_MSG. Get_Text().substr( 0, 3 ) != "esc"  ){
                Thread_Received_Length = recv( Thread_Accepting_Socket, Thread_Receiveng_buf, THREAD_PORT_RECEIVING_BUF_LENGTH, 0);

                if(Thread_Received_Length < 0){
                    close(Thread_Listening_Socket);
                    close(Thread_Accepting_Socket);
                    this->Number_Active_Threads--;
                    this->Thread_Permits[Thread_ID] = 0;
                }

                Thread_Receiveng_buf[Thread_Remote_Length] = '\0';
                User_MSG.Deserialize(Thread_Receiveng_buf);

                if( User_MSG.Get_Text() == "lgn" ) {
                    
                    SRV_MSG.Set_Text( "OK" );

                    sprintf( Thread_Sending_Buf, "%s" , SRV_MSG.Serialize().c_str() );

                    if( send( Thread_Accepting_Socket, Thread_Sending_Buf, strlen(Thread_Sending_Buf), 0) < 0 ){
                        close( Thread_Listening_Socket );
                        close( Thread_Accepting_Socket);
                        this->Number_Active_Threads--;
                        this->Thread_Permits[ Thread_ID ] = 0;
                        return;
                    }

                    Thread_Received_Length = recv( Thread_Accepting_Socket, Thread_Receiveng_buf, THREAD_PORT_RECEIVING_BUF_LENGTH, 0);
                    
                    if( Thread_Received_Length < 0){
                        close( Thread_Listening_Socket );
                        close( Thread_Accepting_Socket);
                        this->Number_Active_Threads--;
                        this->Thread_Permits[ Thread_ID ] = 0;
                        return;
                    }

                    Thread_Receiveng_buf[ Thread_Received_Length ] = '\0';

                    Login_Request User_LGN_Request;
                    User_LGN_Request.Deserialize( Thread_Receiveng_buf );

                    std::string Username = User_LGN_Request.Get_Username();
                    std::string Password = User_LGN_Request.Get_Password();

                    //controllo che l'utente che sta provando a loggarsi esista dentro il database, se essite gli mando i 2 tokens
                    if( Thread_DB_Connection.check_user_exist( Username, Password ) == true){
                        
                        std::vector<std::string> tokens = Thread_DB_Connection.generate_tokens();
                        Login_Request SRV_LGN_Response( tokens[0], tokens[1] );

                        sprintf( Thread_Sending_Buf, "%s", SRV_LGN_Response.Serialize().c_str() );

                        if( send( Thread_Accepting_Socket, Thread_Sending_Buf, strlen(Thread_Sending_Buf), 0) < 0 ){
                            std::cerr<<"sendig the login credential to the client has FAILED: "<<"\n";
                            this->Number_Active_Threads--;
                            this->Thread_Permits[ Thread_ID ] = 0;
                            close( Thread_Listening_Socket );
                            close( Thread_Accepting_Socket );
                            return;
                        }


                    }else /*se l'Utente non esste, o la password è sbagliata*/{
                        Login_Request SRV_LGN_Response;

                        sprintf( Thread_Sending_Buf, "%s", SRV_LGN_Response.Serialize().c_str() );

                        if( send( Thread_Accepting_Socket, Thread_Sending_Buf, strlen(Thread_Sending_Buf), 0) < 0 ){
                            std::cerr<<"sendig the login credential to the client has FAILED: "<<"\n";
                            this->Number_Active_Threads--;
                            this->Thread_Permits[ Thread_ID ] = 0;
                            close( Thread_Listening_Socket );
                            close( Thread_Accepting_Socket );
                            return;
                        }

                    }
                    
                }else if( User_MSG.Get_Text() == "rgs")  {
                    SRV_MSG.Set_Text( "OK" );

                    sprintf( Thread_Sending_Buf, "%s" , SRV_MSG.Serialize().c_str() );

                    if( send( Thread_Accepting_Socket, Thread_Sending_Buf, strlen(Thread_Sending_Buf), 0) < 0 ){
                        close( Thread_Listening_Socket );
                        close( Thread_Accepting_Socket);
                        this->Number_Active_Threads--;
                        this->Thread_Permits[ Thread_ID ] = 0;
                        return;
                    }

                    Thread_Received_Length = recv( Thread_Accepting_Socket, Thread_Receiveng_buf, THREAD_PORT_RECEIVING_BUF_LENGTH, 0);
                    
                    if( Thread_Received_Length < 0){
                        close( Thread_Listening_Socket );
                        close( Thread_Accepting_Socket);
                        this->Number_Active_Threads--;
                        this->Thread_Permits[ Thread_ID ] = 0;
                        return;
                    }

                    Thread_Receiveng_buf[ Thread_Received_Length ] = '\0';
                        
                    Register_Request User_RGS_Request;
                    User_RGS_Request.Deserialize( Thread_Receiveng_buf );

                    
                    if( Thread_DB_Connection.check_username_valid( User_RGS_Request.Get_Username()  ) == true ){
                        Register_Response SRV_RGS_Response;

                        if( Thread_DB_Connection.create_user( User_RGS_Request.Get_Username(), User_RGS_Request.Get_Password() ) < 0 ){
                            SRV_RGS_Response.Set_Auth_Token("OK");
                            SRV_RGS_Response.Set_Refresh_Token("OK");
                        }

                        sprintf( Thread_Sending_Buf, "%s", SRV_RGS_Response.Serialize().c_str() );

                        if( send( Thread_Accepting_Socket, Thread_Sending_Buf, strlen(Thread_Sending_Buf), 0 ) < 0 ){
                            std::cerr<<"sending the confirmation of the user being created has failed"<<"\n";
                            close( Thread_Listening_Socket );
                            close( Thread_Accepting_Socket );
                            this->Number_Active_Threads--;
                            this->Thread_Permits[ Thread_ID ] = 0;
                            return;
                        }

                    }else{
                         Register_Response SRV_RGS_Response;

                         sprintf( Thread_Sending_Buf,  "%s", SRV_RGS_Response.Serialize().c_str() );

                        if( send( Thread_Accepting_Socket, Thread_Sending_Buf,  strlen( Thread_Sending_Buf ), 0 ) < 0){
                            std::cerr<<"letting the user know that username is invalid failed"<<"\n";
                            close( Thread_Listening_Socket );
                            close( Thread_Accepting_Socket );
                            this->Number_Active_Threads--;
                            this->Thread_Permits[ Thread_ID ] = 0;
                            return;                            
                        }

                    }

                }else if( Thread_DB_Connection.check_token_validity( User_MSG.Get_Auth_Token() , User_MSG.Get_Receiver_Username() ) == true){
                    
                }

            }

        }

        int Find_New_Client_Port(){
            for(int i = 0 ; i<MAX_THREADS ; i++){
                if( this->Thread_Permits[i] == 0){
                    return i;
                }
            }

            return -1;
        }

        void Accept_Client(){

            while(1){
                this->Default_Port_Remote_Length = sizeof(this->Default_Port_Remote);
                if(this->Number_Active_Threads < MAX_THREADS){
                    
                    this->Default_Port_Accept_Socket = accept(this->Default_Port_Listening_Socket, (struct sockaddr*)&this->Default_Port_Remote, &this->Default_Port_Remote_Length);
                   
                    if( this->Default_Port_Accept_Socket < 0){
                        close(this->Default_Port_Listening_Socket);
                        exit(-1);
                    }

                    int Thread_ID = Find_New_Client_Port();

                    int New_Client_Port = Thread_ID + 3000;

                    Thread_Permits[Thread_ID] = 1;
                    SRV_Threads[Thread_ID] = std::thread( &Server::Client_Game, this, Thread_ID);

                    sprintf(this->Default_Port_Snd_Buf, "%d", New_Client_Port);

                    if( send(this->Default_Port_Accept_Socket, this->Default_Port_Snd_Buf, strlen(this->Default_Port_Snd_Buf), 0) < 0 ){
                        perror("Sending back the New Client Port to the Client has Failed");
                        close(this->Default_Port_Accept_Socket);
                        close(this->Default_Port_Listening_Socket);
                        exit(-1);
                    }

                    Number_Active_Threads++;
                    
                }
            }
        }

    public:
        Server(){

            this->Default_Port_Local.sin_family = AF_INET;
            this->Default_Port_Local.sin_port = htons(DEFAULT_SERVER_PORT);
            this->Default_Port_Local.sin_addr.s_addr = htonl(INADDR_ANY);

            this->Default_Port_Listening_Socket = socket(AF_INET, SOCK_STREAM, 0);
            
            if(this->Default_Port_Listening_Socket < 0){
                perror("Socket Creation of the Default Port Failed ");
                exit(-1);
            }

            if( bind( this->Default_Port_Listening_Socket, (struct sockaddr*)&this->Default_Port_Local, sizeof(this->Default_Port_Local) < 0)){
                perror("The bind of the Default Port Failed ");
                close(this->Default_Port_Listening_Socket);
                exit(-1);
            }

            listen( this->Default_Port_Listening_Socket, MAX_THREADS);
            Accept_Client();
        }

};

int main(){

    return 0;
}

