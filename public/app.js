'use strict';

const stage = new createjs.Stage('myCanvas');
const socket = io();

const draw = () => {
    const bmp = new createjs.Bitmap(`/img.jpeg?date=${new Date()}`);
    stage.addChild(bmp);
    createjs.Ticker.on('tick', () => stage.update());
    console.log(`update: ${new Date()}`);
}

socket.on('new message', (msg) => draw());

draw(); //初期実行