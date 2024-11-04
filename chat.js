let selectedChat = ""; 


function sendMessage() {
    const messageInput = document.getElementById("messageInput");
    const message = messageInput.value;
    addMessage(message);
    if (message && selectedChat) {
        fetch("/sendMessage", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
            body: `sender=${username}&receiver=${selectedChat}&message=${message}`
        })
        .then(response => {
            if (response.ok) {
                messageInput.value = ""; // Pulisci il campo di input dopo l'invio
                getAllMessages(); // Recupera i messaggi aggiornati
            } else {
                console.error("Errore nell'invio del messaggio");
            }
        })
        .catch(error => console.error("Errore nella comunicazione con il server:", error));
    } else {
        alert("Seleziona un destinatario e scrivi un messaggio.");
    }
}

function addMessage(message){
    const msgbox = document.getElementById("messagesContainer");
    const newMexBox = document.createElement("div");
    newMexBox.classList.add("message_row");

    const newMex = document.createElement("div");
    newMex.classList.add("user_message_box");

    newMex.innerText = message.message;
    newMexBox.appendChild(newMex);
    msgbox.appendChild(newMexBox);

}

function getAllMessages() {
    if (!selectedChat) {
        console.warn('Nessuna chat selezionata. Assicurati di selezionare un destinatario.');
        return; // Esci dalla funzione se nessuna chat è selezionata
    }

    fetch(`/getAllMessages`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: `username=${username}&receiver=${selectedChat}`
    })
    .then(response => {

        if (!response.ok) {
            console.error("Errore nel recupero dei messaggi:", response.statusText);
            return;
        }
        return response.json(); 
    })
    .then(data => {
        printMessages(data);
    })
    .catch(error => console.error("Errore nel recupero dei messaggi:", error));
}

function printMessages(data) {
    const messages = data.messages;

    const messagesContainer = document.getElementById("messagesContainer");
    messagesContainer.innerHTML = ""; 

    if (messages.length === 0) {
        
        const noMessagesElement = document.createElement("div");
        noMessagesElement.classList.add("errorMessage");
        const errorMex = document.createElement("pre");
        errorMex.classList.add("errorPre");

        errorMex.innerText = "Nessun messaggio trovato per questa chat.";
        noMessagesElement.appendChild(errorMex)
        messagesContainer.appendChild(noMessagesElement);
        return;
    } 

    for(let i = 0 ; i<messages.length ; i++){
        const newMexBox = document.createElement("div");
        console.log(messages[i]);
        const newMex = document.createElement("pre");

        console.log(username)
        if( messages[i].sender == username ){
            newMexBox.classList.add("userMessage");
            newMex.classList.add("userPre")
        }else{
            newMexBox.classList.add("chatMessage");
            newMex.classList.add("chatPre");
        }

        newMex.innerText = messages[i].message;
        newMexBox.appendChild(newMex);
        messagesContainer.appendChild(newMexBox);
    }

    
}

function createReceiverButtons(data) {

    const messages = data.messages;
    const receiverSet = new Set(); 

    messages.forEach(message => {
        if (message.sender !== username) { 
            receiverSet.add(message.sender); 
        } else {
            receiverSet.add(message.receiver); 
        }
    });

    populateReceiverButtons(Array.from(receiverSet)); 
}

function selectChat(receiver) {
    selectedChat = receiver; 
    document.getElementById("selectedChatNamePre").innerText = selectedChat;
    document.getElementById("messagesContainer").innerHTML = ""; 
    getAllMessages(); 
}

function getAllUsers() {
    fetch("/getUsers")
        .then(response => {
            if (!response.ok) {
                console.error("Errore nel recupero degli utenti:", response.statusText);
                return;
            }
            return response.json();
        })
        .then(data => {
            populateReceiverButtons(data.users); // Popola i pulsanti con i dati degli utenti
        })
        .catch(error => console.error("Errore nel recupero degli utenti:", error));
}

function initialize() {
    document.getElementById("usernameDisplayer").innerText = "Benvenuto " + username + "!!!";
    getAllMessages(); 
    getAllUsers(); 
    setInterval(getAllMessages, 2000); 
}

function populateReceiverButtons(users) {
    const receiverButtonsContainer = document.getElementById("receiverButtons");
    receiverButtonsContainer.innerHTML = ""; 
    console.log(users);
    users.forEach(user => {
        const button = document.createElement("button");
        button.classList.add('chatSelector')
        button.innerText = user.username || user; 
        button.onclick = () => selectChat(user.username || user);
        receiverButtonsContainer.appendChild(button);
        receiverButtonsContainer.appendChild(document.createElement("br"));
    });
}


initialize();