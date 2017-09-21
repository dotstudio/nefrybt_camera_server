'use strict';

const fs = require('fs');
const app = require('express')();
const http = require('http').Server(app);
const io = require('socket.io')(http);
const express = require('express');
const PORT = process.env.PORT || 3000;
app.use(express.static(__dirname));

const {promisify} = require('util');
const writeFileAsync = promisify(fs.writeFile);

io.on('connection', (socket) => console.log('a user connected')); //socket.ioのコネクション

app.get('/', (req, res) => res.sendFile('./index.thml'));
app.post('/', (req, res) => {
    let buffers = [];
    let cur = 0;
    const len = parseInt(req.headers['content-length'], 10);

    req.on('data', (chunk) => {
        buffers.push(chunk);
        cur += chunk.length;
        console.log(`Downloading...${(100.0 * cur / len).toFixed(2)}%`);
    });

    req.on('end', async () => {
        console.log(`\n[done] Image upload`);
        req.rawBody = Buffer.concat(buffers);
        const base64image = req.rawBody.toString('base64'); //base64変換
        await writeFileAsync('./img.jpeg', req.rawBody, 'utf-8');
        console.log(`[done] Image Save`);
        io.sockets.emit('new image',base64image); //画像送信
    });
    
});
 
http.listen(PORT, () => console.log(`listening on *:${PORT}`));