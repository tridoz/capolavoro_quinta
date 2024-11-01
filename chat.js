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
    fetch("/getAllMessages", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
        },
        body: `username=${username}`
    })
    .then(response => response.json())
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

    messages.forEach(message => {
        const messageElement = document.createElement("div");
        messageElement.innerText = `${message.timestamp} - ${message.sender}: ${message.message}`;
        messagesContainer.appendChild(messageElement);
    });
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

    const receiverButtonsContainer = document.getElementById("receiverButtons");
    receiverButtonsContainer.innerHTML = ""; // Pulisci i pulsanti precedenti

    receiverSet.forEach(receiver => {
        const button = document.createElement("button");
        button.innerText = receiver;
        button.onclick = () => selectChat(receiver); // Imposta il destinatario selezionato
        receiverButtonsContainer.appendChild(button);
    });
}

// Funzione per selezionare una chat
function selectChat(receiver) {
    selectedChat = receiver; // Imposta il destinatario selezionato
    document.getElementById("messagesContainer").innerHTML = ""; // Pulisci i messaggi
    getAllMessages(); // Recupera i messaggi per il destinatario selezionato
}


function getAllUsers() {
    fetch("/getUsers")
        .then(response => response.json())
        .then(data => {
            console.log(data);
            populateReceiverButtons(data.users); // Populate buttons with user data
        })
        .catch(error => console.error("Errore nel recupero degli utenti:", error));
}

// Update the initialize function to also call getAllUsers
function initialize(usernameFromServer) {
    username = usernameFromServer; // Imposta lo username
    getAllMessages(); // Recupera i messaggi iniziali
    getAllUsers(); // Recupera la lista degli utenti
    setInterval(getAllMessages, 2000); // Chiama getAllMessages ogni 2 secondi
}

// New function to populate receiver buttons with users
function populateReceiverButtons(users) {
    const receiverButtonsContainer = document.getElementById("receiverButtons");
    receiverButtonsContainer.innerHTML = ""; // Pulisci i pulsanti precedenti

    users.forEach(user => {
        const button = document.createElement("button");
        button.innerText = user;
        button.onclick = () => selectChat(user); // Imposta il destinatario selezionato
        receiverButtonsContainer.appendChild(button);
    });
}

initialize();
