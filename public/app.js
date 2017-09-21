'use strict';

const socket = io();

const draw = (imageData = '') => {
    const canvas = document.getElementById('myCanvas');
    const ctx = canvas.getContext('2d');
    const img = new Image();
    img.src = `data:image/jpeg;base64,${imageData}`; //基本base64の文字列
    if(imageData === 'init') img.src = `./img.jpeg`; //初期実行時のみサーバーのimg.jpegを取得
    img.onload = () => {
        canvas.width = img.width;
        canvas.height = img.height;
        ctx.drawImage(img, 0, 0, 640, 480);
    }
    console.log(`update ${new Date()}`);
}

socket.on('new image', draw); //画像更新時
draw('init'); //初期実行