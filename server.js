'use strict';

const fs = require('fs');
const app = require('express')();
const http = require('http').Server(app);
const io = require('socket.io')(http);
const express = require('express');
const PORT = process.env.PORT || 3000;
app.use(express.static(__dirname));

io.on('connection', (socket) => console.log('a user connected')); //socket.ioのコネクション

app.get('/', (req, res) => res.sendFile('./index.thml'));
app.post('/', (req, res) => {
    let buffers = [];
    let cnt = 0;

    req.on('data', (chunk) => {
        buffers.push(chunk);
        console.log(++cnt);
    });

    req.on('end', () => {
        console.log(`[done] Image upload`);
        req.rawBody = Buffer.concat(buffers);
        //書き込み
        fs.writeFile('./img.jpeg', req.rawBody, 'utf-8',(err) => {
            if(err) return;
            console.log(`[done] Image save`);
            io.sockets.emit('new message',{message: `[done] Image save`}); //通知
        });
    });
});
 
http.listen(PORT, () => console.log(`listening on *:${PORT}`));