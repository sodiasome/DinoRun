import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtMultimedia 6.4

Window {
    id: window
    width: 900
    height: 350
    visible: true
    title: "恐龙快跑"
    color: isNight ? "#1a1a2e" : "#ffffff"
    readonly property var gameStateEnum: { "Ready": 0, "Running": 1, "Paused": 2, "GameOver": 3 }

    property bool soundEnabled: true
    property real masterVolume: 0.8
    property int gameState: gameManager ? gameManager.gameState : gameStateEnum.Ready
    property int score: gameManager ? gameManager.score : 0
    property int highScore: gameManager ? gameManager.highScore : 0
    property int difficultyLevel: gameManager ? gameManager.difficulty : 1
    property real playerY: gameManager ? gameManager.playerY : 220
    property bool isNight: Math.floor(window.score / 200) % 2 === 1

    Behavior on color {
        ColorAnimation { duration: 500 }
    }

     // 全局触摸输入
    TapHandler {
        enabled: window.gameState === gameStateEnum.Running
        onTapped: {
            gameManager.triggerJump();
        }
    }
    // 全局键盘输入
    Item {
        id: inputArea
        anchors.fill: parent
        focus: true
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Up) {
                if (window.gameState === gameStateEnum.Running) gameManager.triggerJump();
                else if (window.gameState === gameStateEnum.Ready || window.gameState === gameStateEnum.GameOver) gameManager.startGame();
            } else if (event.key === Qt.Key_Escape) {
                if (window.gameState === gameStateEnum.Running) gameManager.pauseGame();
                else if (window.gameState === gameStateEnum.Paused) gameManager.resumeGame();
            }
        }
    }

    // 音频输出
    AudioOutput { id: bgmOutput; volume: window.soundEnabled ? window.masterVolume : 0 }
    MediaPlayer {
        id: bgm
        source: "./sounds/bgm.mp3"
        audioOutput: bgmOutput
        loops: MediaPlayer.Infinite
    }
    SoundEffect { id: jumpSound; source: "./sounds/jump.wav"; volume: window.soundEnabled ? window.masterVolume : 0 }
    SoundEffect { id: hitSound; source: "./sounds/gameOver.wav"; volume: window.soundEnabled ? window.masterVolume : 0 }
    SoundEffect { id: readySound; source: "./sounds/gameStart.wav"; volume: window.soundEnabled ? window.masterVolume : 0 }
    SoundEffect { id: btnSound; source: "./sounds/button.wav"; volume: window.soundEnabled ? window.masterVolume : 0 }
    SoundEffect { id: scoreSound; source: "./sounds/score.wav"; volume: window.soundEnabled ? window.masterVolume : 0 }
    Connections {
        target: gameManager
        onPlayJumpSound: jumpSound.play()
        onPlayHitSound: hitSound.play()
        onPlayBgm: { if (window.soundEnabled) bgm.play(); }
        onPauseBgm: bgm.pause()
        onScoreChanged: {
            if (window.gameState === gameStateEnum.Running && window.score > 0) {
                scoreSound.play();
            }
        }
        onGameStateChanged: {
            if (window.gameState === gameStateEnum.GameOver) {
                bgm.stop();
            }
        }
    }


    // 月亮
    Image {
        id: moonImage
        source: moonFrames[currentMoonIndex]
        x: 300; y: 40
        width: 40; height: 40
        fillMode: Image.PreserveAspectFit

        // 【修改】根据昼夜状态控制显隐和透明度
        opacity: window.isNight ? 0.9 : 0.0
        Behavior on opacity { NumberAnimation { duration: 1000 } }

        property int currentMoonIndex: 0
        readonly property var moonFrames: [
            "./images/moon1.png", "./images/moon2.png", "./images/moon3.png",
            "./images/moon4.png", "./images/moon5.png", "./images/moon6.png", "./images/moon7.png"
        ]
        Timer {
            interval: 5000
            running: window.gameState === gameStateEnum.Running && window.isNight
            repeat: true
            onTriggered: {
                parent.currentMoonIndex = (parent.currentMoonIndex + 1) % parent.moonFrames.length;
            }
        }
    }
    // 浮云
    Item {
        anchors.fill: parent
        Image {
            id: cloud1
            source: "./images/cloud.png"
            x: 100; y:50; width:60; height:20
            NumberAnimation on x { running: window.gameState === gameStateEnum.Running; from: window.width; to: -100; duration:12000; loops: Animation.Infinite }
        }
        Image {
            id: cloud2
            source: "./images/cloud.png"
            x: 500; y:80; width:80; height:25
            NumberAnimation on x { running: window.gameState === gameStateEnum.Running; from: window.width; to: -150; duration:9000; loops: Animation.Infinite }
        }
    }
    // 飞鸟
    Image {
        id: birdObj
        source: "./images/pterosaurs1.png"
        x: 900; y:130; width:35; height:30
        fillMode: Image.PreserveAspectFit
        visible: window.difficultyLevel >= 2 && window.gameState === gameStateEnum.Running
    }
    // 地面
    Rectangle {
        width: parent.width; height: 3; y: 260
        color: window.isNight ? "#ffffff" : "#000000"
        Behavior on color { ColorAnimation { duration: 1000 } }
    }
    // 障碍物
    Repeater {
        model: obstacleModel
        delegate: Item {
            x: model.x
            y: model.y
            width: model.w
            height: model.h

            // 判断当前障碍物是否为飞鸟
            readonly property bool isBird: model.source.toString().indexOf("pterosaurs") !== -1
            property int birdFrame: 1

            // 飞鸟专属的翅膀扇动定时器
            Timer {
                interval: 150 // 每 150 毫秒切换一次翅膀状态
                running: parent.isBird && window.gameState === gameStateEnum.Running
                repeat: true
                onTriggered: {
                    parent.birdFrame = (parent.birdFrame === 1) ? 2 : 1;
                }
            }

            Image {
                id: obsImg
                anchors.fill: parent
                // 如果是飞鸟，在 1 和 2 之间动态切换图片；如果是仙人掌，直接使用模型的 source
                source: parent.isBird ? ("./images/pterosaurs" + parent.birdFrame + ".png") : model.source
                fillMode: Image.PreserveAspectFit
            }
        }
    }
    // 恐龙
    Image {
        id: dinoImage
        x: 80
        y: window.playerY
        width: 40; height: 40
        fillMode: Image.PreserveAspectFit
        source: dinoFrames[currentDinoFrame]

        property int currentDinoFrame: 0
        readonly property var dinoFrames: [
            "./images/dinosaur1.png",
            "./images/dinosaur2.png",
            "./images/dinosaur3.png",
            "./images/dinosaur4.png",
            "./images/dinosaur5.png",
            "./images/dinosaur6.png"
        ]
        Timer {
            interval: 90
            running: window.gameState === gameStateEnum.Running
            repeat: true
            onTriggered: {
                if (dinoImage.y >= 220 - 1 && dinoImage.y <= 220 + 1) {
                    dinoImage.currentDinoFrame = (dinoImage.currentDinoFrame + 1) % dinoImage.dinoFrames.length
                } else {
                    dinoImage.currentDinoFrame = 0
                }
            }
        }
    }
    // HUD
    Rectangle {
        id: hudBg
        width: 220; height: 40
        anchors.top: parent.top; anchors.right: parent.right
        anchors.topMargin: 22; anchors.rightMargin: 14
        color: "#00000055"; radius: 6
        visible: (window.gameState === gameStateEnum.Running || window.gameState === gameStateEnum.Paused)
    }
    Row {
        anchors.right: hudBg.right
        anchors.top: hudBg.top
        anchors.topMargin: 8
        anchors.rightMargin: 10
        spacing: 20
        Text {
            text: "最高分: " + window.highScore;
            color: window.isNight ? "#ffffff" : "#303133";
            font.pixelSize: 16; font.bold: true
        }
        Text {
            text: "分数: " + window.score;
            color: window.isNight ? "#ffffff" : "#303133";
            font.pixelSize: 16; font.bold: true
        }
    }

    // 开始
    Rectangle {
        anchors.fill: parent; color: "#cc000000"
        visible: window.gameState === gameStateEnum.Ready
        ColumnLayout { anchors.centerIn: parent; spacing: 20
            Text { text: "恐龙快跑"; color: "#00ffcc"; font.pixelSize:32; font.bold:true; Layout.alignment: Qt.AlignHCenter }
            Rectangle {
                Layout.preferredWidth: 200; Layout.preferredHeight: 50; Layout.alignment: Qt.AlignHCenter
                color: "#00ffcc"; radius: 6
                Text { anchors.centerIn: parent; text: "开始游戏"; color: "#1a1a1a"; font.pixelSize:18; font.bold:true }
                MouseArea { anchors.fill: parent; onClicked: {
                        console.log("点击开始游戏...");
                        btnSound.play();
                        gameManager.startGame();
                    }
                }
            }

            RowLayout { Layout.alignment: Qt.AlignHCenter; spacing:10
                Text { text: "难度:"; color: "white"; font.pixelSize:14 }
                Repeater {
                    model: ["简单","普通","困难"]
                    Rectangle {
                        Layout.preferredWidth:60; Layout.preferredHeight:30; radius:4
                        color: window.difficultyLevel === (index + 1) ? "#00ffcc" : "#444444"
                        Text { anchors.centerIn: parent; text: modelData; color: window.difficultyLevel === (index + 1) ? "#1a1a1a" : "white"; font.pixelSize:12 }
                        MouseArea { anchors.fill: parent; onClicked: {
                                console.log("点击游戏难度...");
                                readySound.play();
                                btnSound.play();
                                gameManager.setDifficulty(index+1);
                            }
                        }
                    }
                }
            }
        }
    }
    // 暂停
    Rectangle {
        anchors.fill: parent; color: "#aa000000"
        visible: window.gameState === gameStateEnum.Paused
        ColumnLayout { anchors.centerIn: parent; spacing:20
            Text { text: "游戏已暂停"; color:"white"; font.pixelSize:26; font.bold:true; Layout.alignment: Qt.AlignHCenter }

            Rectangle { Layout.preferredWidth:180; Layout.preferredHeight:45; color:"#00ffcc"; radius:6
                Text { anchors.centerIn: parent; text: "继续游戏"; color:"#1a1a1a"; font.bold:true }
                MouseArea { anchors.fill: parent; onClicked: {
                        btnSound.play();
                        gameManager.resumeGame();
                    }
                }
            }
            Rectangle { Layout.preferredWidth:180; Layout.preferredHeight:45; color:"#555555"; radius:6
                Text { anchors.centerIn: parent; text: "返回主菜单"; color:"white"; font.bold:true }
                MouseArea { anchors.fill: parent; onClicked: {
                        console.log("点击返回主菜单...");
                        btnSound.play();
                        gameManager.finishedGame();
                    }
                }
            }
        }
    }
    // 结束界面
    Rectangle {
        anchors.fill: parent; color: "#cc111111"
        visible: window.gameState === gameStateEnum.GameOver
        ColumnLayout { anchors.centerIn: parent; spacing:15
            Text { text: "GAME OVER"; color:"#ff4444"; font.pixelSize:28; font.bold:true; Layout.alignment: Qt.AlignHCenter }
            Text { text: "最终得分: " + window.score; color:"white"; font.pixelSize:18; Layout.alignment: Qt.AlignHCenter }
            RowLayout { Layout.alignment: Qt.AlignHCenter; spacing:15
                Rectangle { Layout.preferredWidth:130; Layout.preferredHeight:40; color:"#00ffcc"; radius:5
                    Text { anchors.centerIn: parent; text:"重新开始"; color:"#1a1a1a"; font.bold:true }
                    MouseArea { anchors.fill: parent; onClicked: { gameManager.startGame(); } }
                }
                Rectangle { Layout.preferredWidth:130; Layout.preferredHeight:40; color:"#444444"; radius:5
                    Text { anchors.centerIn: parent; text:"主菜单"; color:"white"; font.bold:true }
                    MouseArea { anchors.fill: parent; onClicked: {
                            console.log("点击主菜单...");
                            btnSound.play();
                            gameManager.finishedGame();
                        }
                    }
                }
            }
        }
    }
}