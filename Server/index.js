const express = require("express");
const path = require("path");
const socket = require('socket.io');

const logger = require("./middleware/logger");

const app = express();

const schedule = require('node-schedule');

const TeleBot = require("telebot");

//instantiate Telebot with our token got in the BotFather
const bot = new TeleBot({
    // Live token 
    token: "1415793420:AAGVN_VdRITGYXo7pfDanJ3pCYc-JQG1JGA",
    //Test token
    //token: "1519075419:AAFlb411E0iU9_xZqgJ6wEGyp4myvGib3B0",
    polling: true
});

//Firmware bot
const adminBot = new TeleBot({
    token: "1596683372:AAG4vqYlyu8pgyHsPHGitvj2PDrv2PJ5TJc",
    polling: true
});

//Logging
//app.use(logger);

// Get body parser
app.use(express.json());

// Start Listening
const PORT = process.env.PORT || 5000;
var server = app.listen(PORT, () => {
    console.log(`Server started on port ${PORT}`);
});

app.use(express.static('public'));

let io = socket(server);

//start polling
bot.start();

//start polling
adminBot.start();

io.on('connection', (socket) => {
    console.log('Made socket connection', socket.id);

    // adminBot.sendMessage(msg.from.id, `Lamp connected ${socket.id}`);

    //io.emit('id', socket.id);

    socket.on('onConnect', (data) => {
        console.log(data);
    });

    socket.on('disconnect', () => {
        console.log('user disconnected', socket.id);
        // adminBot.sendMessage(msg.from.id, `Lamp disconnected ${socket.id}`);
    });

    //our command
    bot.on(["/broadcast"], (msg) => {
        //all the information about user will come with the msg
        //console.log(msg);
        bot.sendMessage(msg.from.id, 'Saying Hi to everyone!');

        let username = msg.chat.username;
        let broadcast_color;

        if (username == 'vaga_1993' || username == 'MS_K_oz') {
            broadcast_color = { color: 14335 };
            console.log('Broadcasting Blue');
        } else if (username == 'Alexandra' || username == 'Marvin') {
            broadcast_color = { color: 7274751 };
            console.log('Broadcasting Purple');
        } else if (username == 'Jutta' || username == 'Patrick') {
            broadcast_color = { color: 1933312 };
            console.log('Broadcasting Green');
        } else if (username == 'Robert' || username == 'Edith') {
            console.log('Broadcasting Red');
            broadcast_color = { color: 1933312 };
        }

        //To broadcast to all lamps
        io.sockets.emit('broadcastRecieve', broadcast_color);

    });

    bot.on(["/turnoff"], (msg) => {
        let username = msg.chat.username;
        let id;

        console.log('Turning off');

        if (username == 'vaga_1993' || username == 'Sabrina') {
            id = 'B';
        } else if (username == 'Alexandra' || username == 'Marvin') {
            id = 'P';
        } else if (username == 'Jutta' || username == 'Patrick') {
            id = 'G';
        } else if (username == 'Edith' || username == 'Robert') {
            id = 'R';
        }

        bot.sendMessage(msg.from.id, `Turning off your lamp.`);

        io.sockets.emit('turnOff', { id: id });
    });

    bot.on(["/turnon"], (msg) => {
        let username = msg.chat.username;
        let id;

        console.log('Turning on');

        if (username == 'vaga_1993' || username == 'Sabrina') {
            id = 'B';
        } else if (username == 'Alexandra' || username == 'Marvin') {
            id = 'P';
        } else if (username == 'Jutta' || username == 'Patrick') {
            id = 'G';
        } else if (username == 'Edith' || username == 'Robert') {
            id = 'R';
        }

        bot.sendMessage(msg.from.id, `Turning on your lamp.`);

        io.sockets.emit('turnOn', { id: id });
    });


    socket.on('broadcastRequest', (data) => {
        console.log(data);

        let id = String.fromCharCode(data.id);
        let broadcast_color;

        console.log(id);

        if (id == 'T') { // Test 
            //console.log("Test lamp");
            broadcast_color = { color: 14253316 };
        } else if (id == 'G') { // Green
            //console.log("Green lamp");
            broadcast_color = { color: 1933312 };
        } else if (id == 'R') { // Red 
            //console.log("Red lamp");
            broadcast_color = { color: 16064544 };
        } else if (id == 'B') { // Blue 
            //console.log("Blue lamp");
            broadcast_color = { color: 14335 };
        } else if (id == 'P') { // Purple
            //console.log("Purple lamp");
            broadcast_color = { color: 7274751 };
        }

        //To broadcast to all lamps
        io.sockets.emit('broadcastRecieve', broadcast_color);

        // To broadcast to all lamps except the one that sent the broadcast request
        //socket.broadcast.emit('broadcastRecieve', data);
    });

    socket.on('lampConnected', (data) => {
        let id = String.fromCharCode(data.id);
        console.log(id, ' has connected!')
    });

});

adminBot.on(["/update"], (msg) => {
    console.log("Update available");

    adminBot.sendMessage(msg.from.id, 'Update sent out!');

    io.emit('updateAvailable', '1')
});
