// chat.js

const chatArea = document.getElementById("chat-area"); // Assicurati che ci sia un'area per i messaggi
const messageInput = document.getElementById("message-input"); // Input per il messaggio
const sendButton = document.getElementById("send-button"); // Pulsante per inviare il messaggio
const username = "utente1"; // Sostituisci con il nome utente dell'utente loggato
const receiver = "utente2"; // Sostituisci con il destinatario corretto

function updateChat() {
    fetch("/getAllMessages")
        .then(response => response.json())
        .then(messages => {
            chatArea.innerHTML = ""; // Pulisci l'area chat
            messages.forEach(message => {
                const messageElement = document.createElement("div");
                messageElement.textContent = `${message.timestamp} [${message.sender}]: ${message.message}`;
                chatArea.appendChild(messageElement);
            });
        })
        .catch(error => console.error("Errore nel recupero dei messaggi:", error));
}

// Chiama updateChat ogni 2 secondi
setInterval(updateChat, 2000);

sendButton.addEventListener("click", function() {
    const messageText = messageInput.value;
    const timestamp = new Date().toISOString(); // Ottieni il timestamp

    // Invia il messaggio al server
    fetch("/sendMessage", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `sender=${encodeURIComponent(username)}&receiver=${encodeURIComponent(receiver)}&timestamp=${encodeURIComponent(timestamp)}&message=${encodeURIComponent(messageText)}`
    })
    .then(response => {
        if (response.ok) {
            messageInput.value = ""; // Pulisci l'input dopo l'invio
            updateChat(); // Aggiorna la chat dopo l'invio
        } else {
            console.error("Errore nell'invio del messaggio:", response.statusText);
        }
    })
    .catch(error => console.error("Errore nell'invio del messaggio:", error));
});
