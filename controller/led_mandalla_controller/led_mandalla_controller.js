const midi = require('midi');
const apcMiniInput = new midi.Input();
const apcMiniOutput = new midi.Output();
const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');
const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const port = 3000;

init();
function init(){
  var artnetOptions = {
    host: '255.255.255.255',
    iface: '192.168.4.1',
    sendAll: true
  }
  artnet = require('artnet')(artnetOptions);
  patternNumber=56;
  selectedFixtures=[0,1,2,3];
  loopTime=1000;
  copySceneSource=null;
  copySceneTarget=null;
  copyFixtureSource=null;
  copyFixtureTarget=null;
  loopIsPlaying=false;
  apcMiniPort="None";
  fixtureBrightness=[1,1,1,1];
  fixtureMuteMode=0;
  masterBrightness=1;
  isShift=false;
  channels=[];
  loopCellsState=[];
  for(var x=0; x<64; x++){
    var fixtures=[]
    for(var y=0; y<4; y++){
      fixtures.push({
        "colorIndex":127,
        "colorSpread":127,
        "cycleTime":127,
        "trailLength":127,
        "trailSpread":127,
        "dir":127,
        "strobe":0,
        "brightness":127
      });
    }
    channels.push(fixtures);
    loopCellsState[x]=false;
  }
  for(var x=60;x<64;x++){
    loopCellsState[x]=true;
  }
  initMidiConnections();
  recall();
}

function initMidiConnections(){
  for(var x=0; x<apcMiniInput.getPortCount(); x++){
    console.log(apcMiniInput.getPortName(x));
    if(apcMiniInput.getPortName(x).indexOf("APC MINI")!=-1){
      apcMiniPort=x;
      console.log("Found apcMini on port: "+x);
    }
  }
  if(apcMiniPort!="None"){
    apcMiniInput.openPort(apcMiniPort);
    apcMiniInput.ignoreTypes(true,false,true);
    apcMiniOutput.openPort(apcMiniPort);
  }else{
    console.log("A midi device is missing, exiting");
    process.exit(1);
  }
  for(var x=0;x<100;x++){
    apcMiniOutput.sendMessage([144,x,0]);  
  }
  syncLeds();
  setTimeout(function(){syncLeds();},500);
}

apcMiniInput.on('message', (deltaTime, message) => {
  //apc mini grid input
  if(message[0]==144 && message[2]==127){
    //top right button
    if(message[1]==82){
      for(var x=0;x<64;x++){
        if(x!=patternNumber){
          for (var f in selectedFixtures){
            channels[x][selectedFixtures[f]]=JSON.parse(JSON.stringify(channels[patternNumber][selectedFixtures[f]]));
          }
        }
      }
      console.log("Saved Preset To All");
    }
    // top right down 1
    if(message[1]==83){
      for(var y=(Math.floor(patternNumber/4)*4); y<(Math.floor(patternNumber/4)*4)+4;y++){
        if(y!=patternNumber){
          for (var f=0;f<4;f++){
            channels[y][f]=JSON.parse(JSON.stringify(channels[patternNumber][f]));
          }
        }
      }
      console.log("Saved Preset To Row");
    }
    if(message[1]>=0 && message[1]<64 && (message[1]%8)<=3){
      if(!isShift){
        patternNumber=message[1];
        syncArtnetToModel();
        syncLeds();
      }else{
        if(copySceneSource==null){
          copySceneSource=message[1];
          syncLeds();
        }else{
          copySceneTarget=message[1];
          copyScene(copySceneSource,copySceneTarget,[0,1,2,3]);
          copySceneSource=null;
          copySceneTarget=null;
          syncLeds();
        }
      }
    }
    if(message[1]>=0 && message[1]<64 && (message[1]%8)>3){
      loopCellsState[message[1]]=!loopCellsState[message[1]];
      console.log(loopCellsState[x]);
      syncLeds();
    }
    //fixture selectors
    if(message[1]>=64 && message[1]<68 && message[2]==127){
      if(!isShift){
        fixtureButtonPress(message[1]-64);
        syncLeds();
      }else{
        if(copyFixtureSource==null){
          copyFixtureSource=message[1]-64;
          syncLeds();
        }else{
          copyFixtureTarget=message[1]-64;
          copyFixture(copyFixtureSource,copyFixtureTarget);
          copyFixtureSource=null;
          copyFixtureTarget=null;
          syncLeds();
        }
      }
    }
    //start / stop
    if(message[1]==89 && message[2]==127){
      if(!loopIsPlaying){
        loopRoWStart(loopTime);
      }else{
        loopRoWStop();
      }
      syncLeds();
    }
    //save
    if(message[1]==84 && message[2]==127){
      save();
    }
    //recall
    if(message[1]==85 && message[2]==127){
      recall();
    }
  }
  //fixture mute
  if(message[1]>=68 && message[1]<72 && !isShift){      
    if(message[0]==144){
      fixtureBrightness[message[1]-68]=fixtureMuteMode;
    }else{
      fixtureBrightness[message[1]-68]=1-fixtureMuteMode;
    }
    syncArtnetToModel();
    syncLeds();
  }
  // fixture mute mode
  if(message[1]==71 && message[0]==144 && isShift){
    fixtureMuteMode=(fixtureMuteMode+1)%2;
    for(var x=68; x<72; x++){
      fixtureBrightness[x-68]=1-fixtureMuteMode;
    }
    syncArtnetToModel();
    syncLeds();
  }
  //shift on / off
  if(message[1]==98 && message[2]==127){
    if(message[0]==144){
      isShift=true;
    }else{
      isShift=false;
    }
    copySceneSource=null;
    copySceneTarget=null;
    copyFixtureSource=null;
    copyFixtureTarget=null;
    syncLeds();
    console.log(isShift);
  }
  //apc mini sliders input
  if(message[0]==176){
    if(message[1]>=48 && message[1]<=56){
      if(message[1]==48){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].colorIndex=message[2];
        }
      }
      if(message[1]==49){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].colorSpread=message[2];
        }
      }
      if(message[1]==50){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].cycleTime=message[2];
        }
      }
      if(message[1]==51){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].trailLength=message[2];
        }
      }
      if(message[1]==52){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].trailSpread=message[2];
        }
      }
      if(message[1]==53){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].dir=message[2];
        }
      }
      if(message[1]==54){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].strobe=message[2];
        }
      }
      if(message[1]==55){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].brightness=message[2];
        }
      }
      if(message[1]==56 && !isShift){
          loopTime=200+message[2]*50;
          if(loopIsPlaying){
            loopRoWStop();
            loopRoWStart(loopTime);
          }
      }
      if(message[1]==56 && isShift){
        masterBrightness=message[2]/127;
      }
      syncArtnetToModel();
    }
    // console.log(message);
  }
  console.log(message);
});

app.get('/test', (req, res) => {
  for(var x=0;x<64;x++){
    console.log(loopCellsState[x]);
    if((x%8)>3){
      if(loopCellsState[x]){
        apcMiniOutput.sendMessage([144,x,3]); 
      }else{
        apcMiniOutput.sendMessage([144,x,5]); 
      }
    }
  }
  res.send(loopCellsState);
})

app.get('/test2', (req, res) => {
  res.send("Hello World!");
  syncLeds();
  sendArtnet([175,10,255,255,255,255,255,255,255,255,255]);
})

app.get('/test3', (req, res) => {
  copyFixture(0,1);
  res.send("Hello World!3");
})

app.get('/test4', (req, res) => {
  copyScene(56, 57, [0,1,2,3]);
  res.send("Hello World!4");
})

app.get('/save', (req, res) => {
  save();
  res.send(channels);
})

app.get('/recall', (req, res) => {
  recall();
  res.send(JSON.stringify(channels));
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

function recall(){
  fs.readFile('/root/ledControllerData.txt', 'utf8', (err, data) => {
    if (err) {
      console.error(err);
      return;
    }
    console.log(data);
    channels=JSON.parse(data);
    syncArtnetToModel();
  });
}

function save(){
  try {
    fs.writeFileSync('/root/ledControllerData.txt', JSON.stringify(channels));
    console.log('file written successfully');
  } catch (err) {
    console.error(err);
  }
}

function syncLeds(){
  for(var x=0;x<100;x++){
    if(x<64){
      if(!isShift){
        if(x==patternNumber){
          apcMiniOutput.sendMessage([144,x,3]); 
        }else{
          if((x%8)<=3){
            apcMiniOutput.sendMessage([144,x,127]); 
          }else{
            if(loopCellsState[x]){
              apcMiniOutput.sendMessage([144,x,3]); 
            }else{
              apcMiniOutput.sendMessage([144,x,5]); 
            }
          }
        }
      }else{
        if((x%8)<=3){
          if(copySceneSource==x){
            apcMiniOutput.sendMessage([144,x,127]); 
          }else{
            apcMiniOutput.sendMessage([144,x,3]); 
          }
        }else{
          if(loopCellsState[x]){
            apcMiniOutput.sendMessage([144,x,3]); 
          }else{
            apcMiniOutput.sendMessage([144,x,5]); 
          }
        }
      }
    }else{
      if(!(x>=68 && x<72) && x!=86 && x!=87 && x!=88 ){
        apcMiniOutput.sendMessage([144,x,5]);  
      }
    }
    if(x>=64 && x<68){
      if(!isShift){
        if(selectedFixtures.indexOf((x-64))!=-1){
          apcMiniOutput.sendMessage([144,x,127]); 
        }else{
          apcMiniOutput.sendMessage([144,x,0]); 
        }
      }else{
        if((x-64)==copyFixtureSource){
          apcMiniOutput.sendMessage([144,x,127]); 
        }else{
          apcMiniOutput.sendMessage([144,x,0]); 
        }
      }
    }
    if(x>=68 && x<72){
      apcMiniOutput.sendMessage([144,x,127*fixtureBrightness[x-68]]); 
    }
    if(x==89){
      if(!loopIsPlaying){
        apcMiniOutput.sendMessage([144,x,0]); 
      }else{
        apcMiniOutput.sendMessage([144,x,127]); 
      }
    }
  }
}

function sendArtnet(values){
  artnet.set(1,1, values, function (err, res) {
    console.log("Sent Artnet:");
    // console.log(values);
  });
}

function syncArtnetToModel(){
  var temp=[];
  for (var f=0; f<4; f++){
    temp=temp.concat([channels[patternNumber][f].colorIndex*2, channels[patternNumber][f].colorSpread*2, channels[patternNumber][f].cycleTime*2, channels[patternNumber][f].trailLength*2, channels[patternNumber][f].trailSpread*2, channels[patternNumber][f].dir*2, channels[patternNumber][f].strobe*2, masterBrightness*fixtureBrightness[f]*channels[patternNumber][f].brightness*2]);
  }
  sendArtnet(temp);
}

function fixtureButtonPress(val){
  if(selectedFixtures.indexOf(val)!=-1){
      deselectFixture(val);
  }else{
      selectFixture(val);
  }
}

function selectFixture(val){
  selectedFixtures.push(val);
  console.log(selectedFixtures);
}

function deselectFixture(val){
  var temp=[];
  for(var x in selectedFixtures){
      if(selectedFixtures[x]!=val){
          temp=temp.concat(selectedFixtures[x]);
      }
  }
  selectedFixtures=temp;
  console.log(selectedFixtures);
}

function loopRoWStart(freq){
  intervalHandle=setInterval(function(){
    for(var x=1; x<64; x++){
      if(loopCellsState[(patternNumber+4+x)%64]==true){
        patternNumber=((patternNumber+4+x)%64)-4;
        console.log(patternNumber);
        break;
      }
    }
    syncArtnetToModel();
    syncLeds();
  },freq);
  loopIsPlaying=true;
}

function loopRowOld(freq){
  intervalHandle=setInterval(function(){
    if(patternNumber+1>=(Math.floor(patternNumber/4)*4)+4){
      patternNumber=Math.floor(patternNumber/4)*4;
    }else{
      patternNumber=patternNumber+1;
    }
    syncArtnetToModel();
    syncLeds();
  },freq);
  loopIsPlaying=true;
}

function loopRoWStop(){
  clearInterval(intervalHandle);
  loopIsPlaying=false;
}

function copyScene(sourceScene, targetScene, includedFixtures){
  for(var x=0; x<4; x++){
    if(includedFixtures.indexOf(x)!=-1){
      channels[targetScene][x]=JSON.parse(JSON.stringify(channels[sourceScene][x])); 
    }
  }
  syncArtnetToModel();
}

function copyFixture(sourceFixture, targetFixture){
  channels[patternNumber][targetFixture]=JSON.parse(JSON.stringify(channels[patternNumber][sourceFixture])); 
  syncArtnetToModel();
}