const fs = require('fs');
const { exec } = require('child_process');
const midi = require('midi');
const {WebSocketServer} = require('ws');
const wss = new WebSocketServer({ port: 3003 });
const circuitInput = new midi.Input();
const circuitOutput = new midi.Output();
const rolandInput = new midi.Input();
const rolandOutput = new midi.Output();
lkInput = new midi.Input();
lkOutput = new midi.Output();
inControlInput = new midi.Input();
inControlOutput = new midi.Output();
const rtpInput = new midi.Input();
const rtpOutput = new midi.Output();
const express = require('express');
const bodyParser = require('body-parser');
const { channel } = require('diagnostics_channel');
const { emit, kill } = require('process');
const { clear } = require('console');
const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const port = 3000;

init();
function init(){
  internals={
    circuitPort: "None",
    rolandPort: "None",
    lkPort: "None",
    inControlPort: "None",
    rtpPort: "None",
    cursor: 0,
    loopData: [],
    circuitProgramLoopData: [],
    lkLoopData: [],
    loopMaxLength: 96*4,  // in tickts, 6 ticks == 1 quater note == 1 beat
    loopLengths: [96*1, 96*2, 96*4],
    noteOnChannelOne: 144,
    noteOffChannelOne: 128,
    rootDir: "/root/NimbusPi/midiLooper",
    presetsPath: "/presets",
    usb_devices: ""
  }
  globals={
    selectedLength: 0,
    selectedPattern: 1,
    presetName: 0,
    quantize: false,
    transportState: "play",
    programOverride: false,
    programOnly: false,
    eraseEnabled: false
  }
  inctState={
    padMode: 0,
    numberOfPadModes: 4,
    padsOutputChannels: [14,15,16],
    ledPads: [96,97,98,99,112,113,114,115,116,117,118,119,100,101,102,103],
    ledPadsDrumMap: [40,41,42,43,36,37,38,39,44,45,46,47,48,49,50,51],
    ledPadFlexbeatMap: [96,97,98,99,112,113,114,115,116,117,118,119,100,101,102,103],
    ledPadsFlexbeatIndex: [5,6,7,8,1,2,3,4,1,2,3,4,5,6,7,8],
    ledSlicerMap: [96,97,98,99,100,101,102,103,112,113,114,115,116,117,118,119],
    drumPadState: [false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false],
    selectedFlexbeat: [1,1,1,1,1,1],
    flexbeatBank: 0,
    numOfflexbeatBanks: 3,
    currentDrumBank: 0,
    numberOfDrumBanks: 4,
    upCircleButton: [144,104,127],
    downCircleButton: [144,120,127],
    circleDownState: false,
    upArrowButton: [176,104,127],
    downArrowButton: [176,105,127],
    leftArrowButton: [176,106,127],
    rightArrowButton: [176,107,127],
    knobs: [21,22,23,24,25,26,27,28],
    loopCopyShiftState: false
  }
  midiState=[
    [ circuitInput, circuitOutput, "Circuit", "circuitPort", function(){circuitInput.ignoreTypes(true,false,true);} ],
    [ rolandInput, rolandOutput, "Boutiq", "rolandPort", function(){} ],
    [ lkInput, lkOutput, "LK Mini MIDI", "lkPort", function(){} ],
    [ inControlInput, inControlOutput, "InContro", "inControlPort", function(){for(var x=0;x<200;x++){inControlOutput.sendMessage([144,x,127]);}} ],
    [ rtpInput, rtpOutput, "rtpmidid:mpc-one/MPC-rtpmidi mpc-one Network", "rtpPort", function(){} ]
  ]
  function initLoopData(){
    for(var y=0; y<6; y++){
      var temp=[];
      for(var x=0; x<internals.loopMaxLength; x++){
        temp.push([]);
      }
      internals.loopData.push(temp);
    }
  }
  function initCircuitProgramLoopData(){
    for(var y=0; y<6; y++){
      var temp=[];
      for(var x=0; x<internals.loopMaxLength; x++){
        temp.push([]);
      }
      internals.circuitProgramLoopData.push(temp);
    }
  }
  function initLkLoopData(){
    for(var y=0; y<6; y++){
      var temp=[];
      for(var x=0; x<internals.loopMaxLength; x++){
        temp.push([]);
      }
      internals.lkLoopData.push(temp);
    }
  }
  initLoopData();
  initCircuitProgramLoopData();
  initLkLoopData();
  initMidiConnections();
}

inControlInput.on('message', (deltaTime, message) => {
  console.log("inControlInput: "+ message);
  let sendToRtp=true;
  let syncLeds=false;
  let syncWebsocket=false;
  let shouldRecordMessage=false;
  let transformedMessage=message;
  if(message[0]==inctState.upArrowButton[0] && message[1]==inctState.upArrowButton[1] && message[2]==inctState.upArrowButton[2]){ // up arrow button
    inctState.padMode=(inctState.padMode+1)%inctState.numberOfPadModes;
    sendToRtp=false;
    syncLeds=true;
    for(var x=0; x<inctState.drumPadState.length; x++){
      inctState.drumPadState[x]=false;
    }
    console.log("pad mode: "+inctState.padMode);
  }else if(message[0]==inctState.downArrowButton[0] && message[1]==inctState.downArrowButton[1] && message[2]==inctState.downArrowButton[2]){ // down arrow button
    inctState.padMode=(inctState.padMode+(inctState.numberOfPadModes-1))%inctState.numberOfPadModes;
    sendToRtp=false;
    syncLeds=true;
    for(var x=0; x<inctState.drumPadState.length; x++){
      inctState.drumPadState[x]=false;
    }
    console.log("pad mode: "+inctState.padMode); 
  }else if(message[0]==inctState.leftArrowButton[0] && message[1]==inctState.leftArrowButton[1] && message[2]==inctState.leftArrowButton[2]){ // left arrow button
    sendToRtp=false;
    syncLeds=true;
    if(inctState.padMode==0){ // drum bank
      inctState.currentDrumBank=(inctState.currentDrumBank+1)%inctState.numberOfDrumBanks;
      for(var x=0; x<inctState.drumPadState.length; x++){
        inctState.drumPadState[x]=false;
      }
      console.log("current drum bank: "+inctState.currentDrumBank); 
    }
    if(inctState.padMode==1){ // flexbeat bank
      inctState.flexbeatBank=(inctState.flexbeatBank+1)%inctState.numOfflexbeatBanks;
      console.log("flexbeat bank: "+inctState.flexbeatBank);
    }
  }else if(message[0]==inctState.rightArrowButton[0] && message[1]==inctState.rightArrowButton[1] && message[2]==inctState.rightArrowButton[2]){ // right arrow button
    sendToRtp=false;
    syncLeds=true;
    if(inctState.padMode==0){ // drum bank
      inctState.currentDrumBank=(inctState.currentDrumBank+(inctState.numberOfDrumBanks-1))%inctState.numberOfDrumBanks;
      for(var x=0; x<inctState.drumPadState.length; x++){
        inctState.drumPadState[x]=false;
      }
      console.log("current drum bank: "+inctState.currentDrumBank);
    }
    if(inctState.padMode==1){ // flexbeat bank
      inctState.flexbeatBank=(inctState.flexbeatBank+(inctState.numOfflexbeatBanks-1))%inctState.numOfflexbeatBanks;
      console.log("flexbeat bank: "+inctState.flexbeatBank);
    }
  }else if(message[0]==inctState.upCircleButton[0] && message[1]==inctState.upCircleButton[1] && message[2]==inctState.upCircleButton[2]){ // up circle button
    sendToRtp=false;
    syncLeds=true;
    syncWebsocket=true;
    if(!inctState.circleDownState){ // rec vs play toggle
      if(globals.transportState=="play"){
        globals.transportState="rec";
      }else if(globals.transportState=="rec"){
        globals.transportState="play";
      }else if(globals.transportState=="stop"){  
        globals.transportState="play";
      }
    }else{ // clear all for pattern
      clearLoop("all"); 
    }
    console.log("lk up circle changed transport state: "+globals.transportState);
  }else if((message[0]==inctState.downCircleButton[0] || message[0]==inctState.downCircleButton[0]-16) && message[1]==inctState.downCircleButton[1]){ // down circle button
    sendToRtp=false;
    syncLeds=true;
    syncWebsocket=true;
    if(message[2]!=0){
      if(inctState.padMode==0 || inctState.padMode==1 || inctState.padMode==2 || inctState.padMode==3){
        clearLoop("lk"); 
        console.log("lk cleared pattern: "+ globals.selectedPattern);
      }
    }
    if(message[2]==0){
      inctState.circleDownState=false;
      console.log("circleDownState false");
    }else{
      inctState.circleDownState=true;
      console.log("circleDownState true");
    }
    console.log("lk down circle cleared pattern: "+ globals.selectedPattern);
  }else if(inctState.ledPads.includes(message[1])){ // pad buttons
    sendToRtp=true;
    if(inctState.padMode==0){ // drum mode
      shouldRecordMessage=true;
      syncLeds=true;
      inctState.drumPadState[inctState.ledPads.indexOf(message[1])]=(message[2]!=0);
      transformedMessage=[message[0]+inctState.padsOutputChannels[inctState.padMode]-1, inctState.ledPadsDrumMap[inctState.ledPads.indexOf(message[1])]+(inctState.currentDrumBank*16), message[2]];
    }
    if(inctState.padMode==1 && message[2]!=0){ // flexbeat mode
      shouldRecordMessage=true;
      let deck=inctState.flexbeatBank*2;
      for(var x=0; x<inctState.ledPads.length; x++){
        if(x>=8){
          deck=inctState.flexbeatBank*2+1;
        }
        if(inctState.ledPadFlexbeatMap[x]==message[1]){
          inctState.selectedFlexbeat[deck]=inctState.ledPadsFlexbeatIndex[x];
          console.log("selected flexbeat "+deck+": "+inctState.selectedFlexbeat[deck]);
        }
      }
      transformedMessage=[message[0]+inctState.padsOutputChannels[inctState.padMode]-1, inctState.ledPadsDrumMap[inctState.ledPads.indexOf(message[1])]+(inctState.flexbeatBank*16), message[2]];
    }
    if(inctState.padMode==2){ // looper mode
      syncWebsocket=true;
      syncLeds=true;
      sendToRtp=false
      if(message[1]>=inctState.ledPads[0] && message[1]<=inctState.ledPads[3] &&message[2]!=0){ // loops 1-4
        if(inctState.loopCopyShiftState){ // set copy mode target
          let cpTarget=0;
          if(message[1]==inctState.ledPads[0]){
            cpTarget=0;
          }else if(message[1]==inctState.ledPads[1]){
            cpTarget=1;
          }else if(message[1]==inctState.ledPads[2]){
            cpTarget=2;
          }else if(message[1]==inctState.ledPads[3]){
            cpTarget=3;
          } 
          processAction("copy", cpTarget);
          inctState.loopCopyShiftState=false;
          console.log("copied pattern "+globals.selectedPattern+" to pattern "+cpTarget);
        }else{ // normal loop selection
          let newSelectedPatten=0;
          if(message[1]==inctState.ledPads[0]){
            newSelectedPatten=1;
          }else if(message[1]==inctState.ledPads[1]){
            newSelectedPatten=2;
          }else if(message[1]==inctState.ledPads[2]){
            newSelectedPatten=3;
          }else if(message[1]==inctState.ledPads[3]){
            newSelectedPatten=4;
          } 
          if( globals.selectedPattern!=newSelectedPatten || globals.transportState=="stop" ){
            killAllNotes();
            console.log("selected pattern: "+newSelectedPatten);
          }
          globals.selectedPattern=newSelectedPatten;
        }
      }else if(message[1]==inctState.ledPads[14] && message[2]!=0){ //clear circuit
        processAction("clearCircuit");
        console.log("lk cleared circuit pattern");
      }else if(message[1]==inctState.ledPads[15] && message[2]!=0){ // clear roland  
        processAction("clearRoland");
        processAction("clearLk");
        console.log("lk cleared roland pattern");
      }else if(message[1]==inctState.ledPads[13] && message[2]!=0){ // killnotes button  
        processAction("killNotes");
        console.log("lk killed notes");
      }else if(message[1]==inctState.ledPads[10] && message[2]!=0){ // save button  
        processAction("save");
        console.log("lk saved pattern");
      }else if(message[1]==inctState.ledPads[11] && message[2]!=0){ // recall button  
        processAction("recall");
        console.log("lk recalled pattern");
      }else if(message[1]==inctState.ledPads[9] && message[2]!=0){ // reload button  
        processAction("reload");
        console.log("lk reloaded pattern");
      }else if(message[1]==inctState.ledPads[4] && message[2]!=0){ // length button 
        globals.selectedLength=(globals.selectedLength+1)%internals.loopLengths.length;
        console.log("lk changed loop length: "+globals.selectedLength);
      }else if(message[1]==inctState.ledPads[12]){ // copy (shift) button  
        inctState.loopCopyShiftState = (message[2]!=0);
        console.log("lk shift copy: "+inctState.loopCopyShiftState);
      }   
    }
    if(inctState.padMode==3){ // slicer mode
      shouldRecordMessage=false;
      sendToRtp=false;
      syncLeds=true;
      if(message[2]!=0){
        let lengthModifier=internals.loopLengths[globals.selectedLength]/16;
        internals.cursor=(inctState.ledSlicerMap.indexOf(message[1])*lengthModifier)+internals.cursor%6;
        killAllNotes();
      }
    }
  }else if(message[0]==176 && inctState.knobs.includes(message[1])){ // knobs
    sendToRtp=true;
    transformedMessage=[message[0], message[1], message[2]];
  }else{
    sendToRtp=false;
  }
  if(sendToRtp){
    rtpOutput.sendMessage(transformedMessage);
    console.log("inControlInput transformedMessage: "+transformedMessage);
  }
  if(shouldRecordMessage){
    recordMessage(transformedMessage, "lkLoopData");
  }
  if(syncLeds){
    syncLaunchkeyLEDS();
  }
  if(syncWebsocket){
    emitSocketMessage();
  }
});

function syncLaunchkeyLEDS(){
  syncCircleButtonLEDS(39,7,100); // orange, red, green
  if(inctState.padMode==0){
    syncDrumPadLEDS(100, 39);
  }else if(inctState.padMode==1){
    syncFlexbeatOnLEDS(39,7,100,63);
  }else if(inctState.padMode==2){
    syncLooperLEDS(39,7,100);
  }else{
    syncSlicerLEDS(39,7,100);
  }
  function syncCircleButtonLEDS(orange, red, green){
    if(!inctState.circleDownState){
      if(globals.transportState=="play"){
        inControlOutput.sendMessage([inctState.upCircleButton[0],inctState.upCircleButton[1],green]);
      }else if(globals.transportState=="rec"){
        inControlOutput.sendMessage([inctState.upCircleButton[0],inctState.upCircleButton[1],red]);
      }else{
        inControlOutput.sendMessage([inctState.upCircleButton[0],inctState.upCircleButton[1],orange]);
      }
    }else{
      inControlOutput.sendMessage([inctState.upCircleButton[0],inctState.upCircleButton[1],orange]);
    }
    if(inctState.circleDownState){
      inControlOutput.sendMessage([inctState.downCircleButton[0],inctState.downCircleButton[1],red]);
    }else{
      inControlOutput.sendMessage([inctState.downCircleButton[0],inctState.downCircleButton[1],orange]);
    }
  }
  function syncLooperLEDS(orange, red, green){
    let count=0;
    for(var x of inctState.ledPads){
      if(count>=0 && count<=3){ // pattern select buttons
        if(count==globals.selectedPattern-1 ){
          if(inctState.loopCopyShiftState){ // copy/shift mode
            inControlOutput.sendMessage([144,x,orange]);
          }else{
            inControlOutput.sendMessage([144,x,red]);
          }
        }else{
          if(inctState.loopCopyShiftState){ // copy/shift mode
            inControlOutput.sendMessage([144,x,red]);
          }else{
              inControlOutput.sendMessage([144,x,orange]);
          }
        }
      }else if(count == 13 || count == 14 || count == 15){ // clear buttons
        inControlOutput.sendMessage([144,x,orange]);
      }else if(count == 9 || count == 10 || count == 11){ // save/restore buttons
        inControlOutput.sendMessage([144,x,red]);
      }else{        
        inControlOutput.sendMessage([144,x,green]);
      }
      count++;
    }
  }
  function syncFlexbeatOnLEDS(orange, red, green, yellow){
    let count=0;
    for(var x of inctState.ledPadFlexbeatMap){
      let bankA=inctState.selectedFlexbeat[inctState.flexbeatBank*2];
      let bankB=inctState.selectedFlexbeat[1+inctState.flexbeatBank*2];
      let onColor=orange;
      if(inctState.flexbeatBank==1){
        onColor=green;
      }
      if(inctState.flexbeatBank==2){
        onColor=yellow;
      }
      inControlOutput.sendMessage([144,x,onColor]);
      if( inctState.ledPadsFlexbeatIndex[count] == bankA && count<8 || inctState.ledPadsFlexbeatIndex[count] == bankB && count>=8 ){
        inControlOutput.sendMessage([144,x,onColor]);
      }else{
        inControlOutput.sendMessage([144,x,red]);
      }
      count++;
    }
  }
  function syncDrumPadLEDS(ColorOn, ColorOff){
    let count=0;
    for(var x of inctState.ledPads){
      if( inctState.drumPadState[count]){
        inControlOutput.sendMessage([144,x,ColorOn]);
      }else{
        inControlOutput.sendMessage([144,x,ColorOff]);
      }
      count++;
    }
  }
  function syncSlicerLEDS(orange, red, green){
    let count=0;
    let lengthModifier=internals.loopLengths[globals.selectedLength]/16;
    for(var x of inctState.ledSlicerMap){
      if( count == (Math.floor(internals.cursor/lengthModifier)) && globals.transportState!="stop" ){
        inControlOutput.sendMessage([144,x,green]);
      }else{
        inControlOutput.sendMessage([144,x,0]);
      }
      count++;
    }
  }
}

function clearLoop(scope){
  if(scope=="roland" || scope == "all"){
    for(var x=0; x<internals.loopMaxLength; x++){
      internals.loopData[globals.selectedPattern-1][x]=[];
    }
  }
  if(scope=="circuit" || scope == "all"){
    for(var x=0; x<internals.loopMaxLength; x++){
      internals.circuitProgramLoopData[globals.selectedPattern-1][x]=[];
    }
  }
  if(scope=="lk" || scope == "all"){
    for(var x=0; x<internals.loopMaxLength; x++){
      internals.lkLoopData[globals.selectedPattern-1][x]=[];
    }
    for(var x=0; x<inctState.drumPadState.length; x++){
      inctState.drumPadState[x]=false;
    }
    for(var x=0; x<inctState.selectedFlexbeat.length; x++){
      inctState.selectedFlexbeat[x]=1;
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 127]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
    } 
    console.log("cleared lk pattern");
  }
  killAllNotes();
}

function killAllNotes(){
  console.log("killing all notes")
  for(var x=48; x<=72; x++){
    // rolandOutput.sendMessage([130,parseInt(x, 16),0]);
    rolandOutput.sendMessage([130,x,0]);
  }
  for(var x=128; x<=131; x++){
    for(var y=0; y<=127; y++){
      rtpOutput.sendMessage([x+16,y,0]);
      rtpOutput.sendMessage([x,y,0]);
    }
  }
}

function recordMessage(message, bufferaName){
  console.log(message);
  if(globals.transportState=="rec" && message[0]!=248 && message[0]!=250 && message[0]!=252 ){
    if(globals.quantize==false){
      if(globals.selectedLength==0){
        internals[bufferaName][globals.selectedPattern-1][internals.cursor].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor+96*1].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor+96*2].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor+96*3].push(message);
      }else if(globals.selectedLength==1){
        internals[bufferaName][globals.selectedPattern-1][internals.cursor].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor+96*2].push(message);
      }else{
        internals[bufferaName][globals.selectedPattern-1][internals.cursor].push(message);
      }
    }else{
      if(globals.selectedLength==0){
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)+96*1].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)+96*2].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)+96*3].push(message);
      }else if(globals.selectedLength==1){   
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)].push(message);
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)+96*2].push(message);
      }else{
        internals[bufferaName][globals.selectedPattern-1][internals.cursor-(internals.cursor%6)].push(message);
      }
    }
  }
}

lkInput.on('message', (deltaTime, message) => {
  if(globals.transportState=="rec" && message[0]!=248 && message[0]!=250 && message[0]!=252 ){
    recordMessage(message,"lkLoopData");
  }
  rtpOutput.sendMessage(message);
  console.log("lkInput: "+ message );
});

circuitInput.on('message', (deltaTime, message) => {
  if(message[0]!=248){
    console.log(message);
  }
  if(message[0]==199 || message[0]==193){
    recordMessage(message,"circuitProgramLoopData");
  }
  if(message[0]==248){
    // console.log(internals.cursor);
    internals.cursor=(internals.cursor+1)%internals.loopLengths[globals.selectedLength];
    if( internals.cursor%6 == 0){
      syncLaunchkeyLEDS();
    }
  }
  if(message[0]==250){
    internals.cursor=0;
    globals.transportState="play";
    console.log("circuit play pressed");
    emitSocketMessage();
  }
  if(message[0]==252){
    globals.transportState="stop";
    killAllNotes();
    console.log("circuit stop pressed");
    emitSocketMessage();
  }
  if(globals.transportState=="play" || globals.transportState=="rec"){
    if(globals.eraseEnabled){
      internals.loopData[globals.selectedPattern-1][internals.cursor]=[];
      internals.circuitProgramLoopData[globals.selectedPattern-1][internals.cursor]=[];
      internals.lkLoopData[globals.selectedPattern-1][internals.cursor]=[];
    }
    for(var x=0; x<internals.loopData[globals.selectedPattern-1][internals.cursor].length; x++){
      var sendMessage=false;
      var playbacMessage=internals.loopData[globals.selectedPattern-1][internals.cursor][x];
      if( playbacMessage[0]==248 || playbacMessage==250 || playbacMessage==252 ){
        sendMessage=false;
      }else if(globals.programOnly && playbacMessage[0]==194){
        sendMessage=true;
      }else if(globals.programOverride && playbacMessage[0]!=194){
        sendMessage=true;
      }else if(!globals.programOverride && !globals.programOnly){
        sendMessage=true;
      }
      if(sendMessage){
        rolandOutput.sendMessage(playbacMessage);
      }
    }
    for(var x=0; x<internals.circuitProgramLoopData[globals.selectedPattern-1][internals.cursor].length; x++){
      var playbacMessage=internals.circuitProgramLoopData[globals.selectedPattern-1][internals.cursor][x];
      circuitOutput.sendMessage(playbacMessage);
    }
    for(var x=0; x<internals.lkLoopData[globals.selectedPattern-1][internals.cursor].length; x++){
      var playbacMessage=internals.lkLoopData[globals.selectedPattern-1][internals.cursor][x];
      if(inctState.padMode==0 && (playbacMessage[0] == inctState.padsOutputChannels[inctState.padMode]+144-1 || playbacMessage[0] == inctState.padsOutputChannels[inctState.padMode]+144-1-16 )){ // drum mode
        let currentVelocity=playbacMessage[2];
        let currentLedIndex=inctState.ledPadsDrumMap.indexOf(playbacMessage[1]-inctState.currentDrumBank*16);
        if(currentLedIndex!=-1 && currentVelocity!=0){
          inctState.drumPadState[currentLedIndex]=true;
        }else if(currentLedIndex!=-1 && currentVelocity==0){
          inctState.drumPadState[currentLedIndex]=false;
        }
      }else if(inctState.padMode==1 && playbacMessage[0] == inctState.padsOutputChannels[inctState.padMode]+144-1){ // flexbeat mode
        let currentVelocity2=playbacMessage[2];
        let currentLedIndex2=inctState.ledPadsDrumMap.indexOf(playbacMessage[1]-inctState.flexbeatBank*16);
        if(currentLedIndex2!=-1 && currentVelocity2!=0){
          if(currentLedIndex2<8){
            inctState.selectedFlexbeat[0+inctState.flexbeatBank*2]=inctState.ledPadsFlexbeatIndex[currentLedIndex2];
          }else{
            inctState.selectedFlexbeat[1+inctState.flexbeatBank*2]=inctState.ledPadsFlexbeatIndex[currentLedIndex2];
          }
        }
      }
      rtpOutput.sendMessage(playbacMessage);
      syncLaunchkeyLEDS();
    }
  }
});

rolandInput.on('message', (deltaTime, message) => {
  recordMessage(message, "loopData");
});

app.get('/action', (req, res) => {
  let action=req.query.value;
  let copyTarget=parseInt(req.query.copyTarget-1);
  processAction(action, copyTarget);
  res.send('received '+action+' cmd')
})

app.get('/getState', (req, res) => {
  console.log('received getState cmd');
  res.send(globals);
})

app.post('/setState', (req, res) => {
  console.log('received setState cmd');
  if( globals.selectedPattern!=parseInt(req.body.selectedPattern) || req.body.transportState=="stop" ){
    killAllNotes();
  }
  globals=req.body;
  res.send(JSON.stringify(req.body));
})

app.get('/debug', (req, res) => {
  res.send(JSON.stringify({"int":internals,"inctState":inctState}));
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

app.get('/gui', function (req, res) {
  const options = {
      root: internals.rootDir + "/"
  };

  const fileName = 'mpanel.html';
  res.sendFile(fileName, options, function (err) {
      if (err) {
          next(err);
      } else {
          console.log('Sent:', fileName);
      }
  });
});

wss.on('connection', function connection(ws) {
  ws.on('message', function message(data) {
    console.log('received: %s', data);
    try{
      //JSON.parse(data)
      console.log('server recieved web socket message');
    }catch{}
  });
  // ws.send('something');
});

function emitSocketMessage(){
  var messagePayload={};
  syncLaunchkeyLEDS();
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(globals));
    }
  });
}

function processAction(action, copyTarget="dummyValue"){
  if(action=="clear"){
    clearLoop("all");
  }
  if(action=="clearRoland"){
    clearLoop("roland");
  }
  if(action=="clearCircuit"){
    clearLoop("circuit");
  }
  if(action=="clearLk"){
    clearLoop("lk");
  }
  if(action=="killnotes"){
    killAllNotes();
  }
  if(action=="recall"){
    fs.readFile(internals.rootDir + internals.presetsPath + parseInt(globals.presetName) + '.txt', 'utf8', (err, data) => {
      if (err) {
        console.error(err);
        return;
      }
      var tempData=JSON.parse(data);
      internals.loopData=tempData.roland;
      internals.circuitProgramLoopData=tempData.circuit;
      internals.lkLoopData=tempData.lk;
      console.log('recalled saved presets!');
    });
  }
  if(action=="save"){
    var fileContents=JSON.stringify({ "roland": internals.loopData, "circuit": internals.circuitProgramLoopData, "lk": internals.lkLoopData });
    fs.writeFile(internals.rootDir + internals.presetsPath + parseInt(globals.presetName) + '.txt', fileContents, err => {
      if (err) {
        console.error(err);
      }
      console.log('saved preset data!');
    });
  }
  if(action=="copy"){
    internals.loopData[copyTarget]=JSON.parse(JSON.stringify(internals.loopData[globals.selectedPattern-1]));
    internals.circuitProgramLoopData[copyTarget]=JSON.parse(JSON.stringify(internals.circuitProgramLoopData[globals.selectedPattern-1]));
    internals.lkLoopData[copyTarget]=JSON.parse(JSON.stringify(internals.lkLoopData[globals.selectedPattern-1]));
  }
  if(action=="reload"){
    dir = exec("sudo /usr/sbin/service mtest restart", function(err, stdout, stderr) {
      if (err) {
        console.log(err);
      }
      console.log(stdout);
    });
  }
}

function checkUSBDevices(){
  dir = exec("sudo amidi -l", function(err, stdout, stderr) {
    if (err) {
      internals.usb_devices = err;
    }else{
      internals.usb_devices = stdout;
    }
    for(var x=0; x<midiState.length; x++){
      if((internals.usb_devices.indexOf(midiState[x][2])==-1 || internals[midiState[x][3]]=="None") && midiState[x][2].indexOf("rtpmid")==-1){ 
        initMidiDevice(midiState[x][0], midiState[x][1], midiState[x][2], midiState[x][3], midiState[x][4]);
      }
    }
  });
  setTimeout(checkUSBDevices, 5000);
}

function initMidiConnections(){
  console.log(" ");
  console.log("Searching for MIDI Devices:");
  console.log("--------------------------");
  for(var x=0; x<midiState.length; x++){
    initMidiDevice(midiState[x][0], midiState[x][1], midiState[x][2], midiState[x][3], midiState[x][4]);
  }
  killAllNotes();
  console.log("--------------------------");
  console.log(" ");
  checkUSBDevices();
}

function initMidiDevice(midiInObject, midiOutObject, deviceString, portName, specialtyFunc){
  if(internals[portName]!="None"){
    midiInObject.closePort();
    midiOutObject.closePort();
    internals[portName]="None";
  }
  for(var x=0; x<midiInObject.getPortCount(); x++){
    if(midiInObject.getPortName(x).indexOf(deviceString)!=-1){
      internals[portName]=x;
      console.log("FOUND "+deviceString+" ON PORT: "+x);
    }
  }
  if(internals[portName]!="None"){
    midiInObject.openPort(internals[portName]);
    midiOutObject.openPort(internals[portName]);
    specialtyFunc();
  }
}
