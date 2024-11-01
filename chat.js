let selectedChat = ""; // Variabile globale per il destinatario selezionato
let username = ""; // Variabile per lo username dell'utente loggato

// Funzione per inviare un messaggio al server
function sendMessage() {
    const messageInput = document.getElementById("messageInput");
    const message = messageInput.value;

    if (message && selectedChat) {
        fetch("/sendMessage", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
            body: `sender=${username}&receiver=${selectedChat}&message=${encodeURIComponent(message)}`
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

// Funzione per ottenere tutti i messaggi dal server
function getAllMessages() {
    if (!selectedChat) {
        console.warn('Nessuna chat selezionata. Assicurati di selezionare un destinatario.');
        return; // Esci dalla funzione se nessuna chat è selezionata
    }

    fetch(`/getAllMessages?chatId=${selectedChat}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: `username=${username}`
    })
    .then(response => {
        // Controlla se la risposta è valida e non contiene errori
        if (!response.ok) {
            console.error("Errore nel recupero dei messaggi:", response.statusText);
            return;
        }
        return response.json(); // Analizza come JSON
    })
    .then(data => {
        printMessages(data);
        createReceiverButtons(data); // Crea i pulsanti dei destinatari
    })
    .catch(error => console.error("Errore nel recupero dei messaggi:", error));
}

// Funzione per stampare i messaggi
function printMessages(messages) {
    const messagesContainer = document.getElementById("messagesContainer");
    messagesContainer.innerHTML = ""; // Pulisci i messaggi precedenti

    if (messages.length === 0) {
        const noMessagesElement = document.createElement("div");
        noMessagesElement.innerText = "Nessun messaggio trovato per questa chat.";
        messagesContainer.appendChild(noMessagesElement);
    } else {
        messages.forEach(message => {
            const messageElement = document.createElement("div");
            messageElement.innerText = `${message.timestamp} - ${message.sender}: ${message.message}`;
            messagesContainer.appendChild(messageElement);
        });
    }
}

// Funzione per creare pulsanti dei destinatari
function createReceiverButtons(messages) {
    const receiverSet = new Set(); // Usa un Set per evitare duplicati

    messages.forEach(message => {
        if (message.sender !== username) { // Solo destinatari che non sono l'utente loggato
            receiverSet.add(message.sender); // Aggiungi il mittente alla lista dei destinatari
        } else {
            receiverSet.add(message.receiver); // Aggiungi il ricevente se l'utente è il mittente
        }
    });

    populateReceiverButtons(Array.from(receiverSet)); // Passa l'array di destinatari
}

// Funzione per selezionare una chat
function selectChat(receiver) {
    selectedChat = receiver; // Imposta il destinatario selezionato
    document.getElementById("messagesContainer").innerHTML = ""; // Pulisci i messaggi
    getAllMessages(); // Recupera i messaggi per il destinatario selezionato
}

// Funzione per ottenere tutti gli utenti
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
    username = getUsername(); 
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
        button.innerText = user.username || user; 
        button.onclick = () => selectChat(user.username || user);
        receiverButtonsContainer.appendChild(button);
        receiverButtonsContainer.appendChild(document.createElement("br"));
    });
}

function getUsername(){
    fetch('/getUsername')
    .then(response => {
        // Log the raw response for debugging
        return response.text().then(text => {
            console.log('Raw response:', text);  // Log the raw response
            return JSON.parse(text);  // Then try to parse it as JSON
        });
    })
    .then(data => {
        console.log('Logged in username:', data.username);
        return data.username;
    })
    .catch(error => console.error('Error fetching username:', error));
}

initialize();