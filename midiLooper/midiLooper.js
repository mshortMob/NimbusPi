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
const { emit, kill, send } = require('process');
const { clear } = require('console');
const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const port = 3000;

init();
function init(){
  config={
    rootDir: "/root/NimbusPi/midiLooper",
    presetsPath: "/presets",
    rtpResetScriptPath: "/scripts/restartRtpMidi.sh",
    guiPath: "/midiLooper.html",
    systemdServiceName: "midiLooper"
  }
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
    usb_devices: "",
    last_rtp_reset_time: 0
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
    looperPadState: [false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false],
    selectedFlexbeat: [1,1,1,1,1,1],
    flexbeatBank: 0,
    numOfflexbeatBanks: 3,
    currentDrumBank: 0,
    numberOfDrumBanks: 4,
    drumRepeatBanks: [2,3],
    drumRepeatFreqs: [6,3], // in steps, 6 steps == 1 beat
    drumRepeatIntervals: [24,18,12,9,6,3,2,1],
    drumRepeatValues: [4,4],
    drumRepeatState: [],
    repeatControlsLedMap: [112,113,114,115,96,97,98,99],
    repeatControlsLedMapTransformed: [36,37,38,39,40,41,42,43],
    repeatControlsLedPadsFullMap: [112,113,114,115,96,97,98,99,116,117,118,119,100,101,102,103],
    looperModeBank: 0,
    numOfLooperModeBanks: 2,
    looperModeDeckValues: [0,0],
    mostRecentUpdatedSynthChan: 0,
    upCircleButton: [144,104,127],
    downCircleButton: [144,120,127],
    circleDownState: false,
    upArrowButton: [176,104,127],
    downArrowButton: [176,105,127],
    leftArrowButton: [176,106,127],
    rightArrowButton: [176,107,127],
    knobs: [21,22,23,24,25,26,27,28],
    loopCopyShiftState: false,
    midiNotesState: []
  }
  stutterValues=[
    { "mix": 0,   "steps": 2,  "freeze": 0, "steplength": 127, "interval": 127 },
    { "mix": 127, "steps": 2,  "freeze": 0, "steplength": 127, "interval": 127 },
    { "mix": 127, "steps": 16, "freeze": 0, "steplength": 127, "interval": 127 },
    { "mix": 127, "steps": 2,  "freeze": 0, "steplength": 127, "interval": 96  },
    { "mix": 127, "steps": 2,  "freeze": 0, "steplength": 64,  "interval": 96  },
    { "mix": 127, "steps": 16, "freeze": 0, "steplength": 96,  "interval": 96  },
    { "mix": 127, "steps": 16, "freeze": 0, "steplength": 56,  "interval": 96  },
    { "mix": 127, "steps": 16, "freeze": 0, "steplength": 24,  "interval": 64  }
  ],
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
    for(var x=0; x<=4; x++){
      var temp=[];
      for(var y=0; y<=127; y++){
        temp.push(false);
      }
      inctState.midiNotesState.push(temp);
    }
  }
  function initdrumRepeatState(){
    for(var x=0; x<inctState.drumRepeatBanks.length; x++){
      var temp=[];
      for(var y=0; y<16; y++){
        temp.push(0);
      }
      inctState.drumRepeatState.push(temp);
    }
  }
  initLoopData();
  initCircuitProgramLoopData();
  initLkLoopData();
  initdrumRepeatState();
  initMidiConnections();
  init
}

function syncLaunchkeyLEDS(){
  syncCircleButtonLEDS(39,7,100); // orange, red, green
  if(inctState.padMode==0){
    syncDrumPadLEDS(100, 35, 39, 7, 100,63);
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
      if(inctState.looperModeBank==0){ // first looper bank, looper controls
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
        }else if(count == 4 || count == 5 || count == 6 || count == 7){ // length buttons
          if(count==4){
            if(globals.selectedLength==0){
              inControlOutput.sendMessage([144,x,red]);
            }else if(globals.selectedLength==1){
              inControlOutput.sendMessage([144,x,red]);
            }else if(globals.selectedLength==2){
              inControlOutput.sendMessage([144,x,red]);
            }
          }else if(count==5){
            if(globals.selectedLength==0){
              inControlOutput.sendMessage([144,x,orange]);
            }else if(globals.selectedLength==1){
              inControlOutput.sendMessage([144,x,red]);
            }else if(globals.selectedLength==2){
              inControlOutput.sendMessage([144,x,red]);
            }
          }else if(count==6 || count==7){
            if(globals.selectedLength==0){
              inControlOutput.sendMessage([144,x,orange]);
            }else if(globals.selectedLength==1){
              inControlOutput.sendMessage([144,x,orange]);
            }else if(globals.selectedLength==2){
              inControlOutput.sendMessage([144,x,red]);
            }
          }
        }else{        
          inControlOutput.sendMessage([144,x,green]);
        }
      }
      if(inctState.looperModeBank==1){ // second looper bank, stutter
        var deck=count>=8 ? 1 : 0;
        if(inctState.looperModeDeckValues[deck]==count%8){
          inControlOutput.sendMessage([144,inctState.repeatControlsLedPadsFullMap[count],red]);
        }else{
          inControlOutput.sendMessage([144,inctState.repeatControlsLedPadsFullMap[count],green]);
        }
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
      let offColor=green;
     if(inctState.flexbeatBank==0){
        onColor=orange;
        offColor=red;
      }
      if(inctState.flexbeatBank==1){
        onColor=green;
        offColor=red;
      }
      if(inctState.flexbeatBank==2){
        onColor=yellow;
        offColor=green;
      }
      inControlOutput.sendMessage([144,x,onColor]);
      if( inctState.ledPadsFlexbeatIndex[count] == bankA && count<8 || inctState.ledPadsFlexbeatIndex[count] == bankB && count>=8 ){
        inControlOutput.sendMessage([144,x,onColor]);
      }else{
        inControlOutput.sendMessage([144,x,offColor]);
      }
      count++;
    }
  }
  function syncDrumPadLEDS(ColorOn, ColorOff, orange, red, green, yellow){
    if(!inctState.drumRepeatBanks.includes(inctState.currentDrumBank)){ // normal non-repeat drum bank
      let count=0;
      for(var x of inctState.ledPads){
        if( inctState.drumPadState[count]){
          inControlOutput.sendMessage([144,x,ColorOn]);
        }else{
          if( count-12 == inctState.currentDrumBank ){
            inControlOutput.sendMessage([144,inctState.ledPads[count],yellow]);
          }else{
            inControlOutput.sendMessage([144,x,ColorOff]);
          }
        }
        count++;
      }
    }else{ // repeater bank with controls section
      let count=0;
      for(var x of inctState.repeatControlsLedPadsFullMap){
        if(inctState.repeatControlsLedMap.includes(x)){ // controls half section
          if(count==inctState.drumRepeatValues[inctState.drumRepeatBanks.indexOf(inctState.currentDrumBank)]){
            inControlOutput.sendMessage([144,x,red]);
          }else{
            inControlOutput.sendMessage([144,x,green]);
          }
        }else{ // normal drum pad half section
          if( inctState.drumPadState[count]){
              inControlOutput.sendMessage([144,x,ColorOn]);
          }else{
            if( count-12 == inctState.currentDrumBank ){
              inControlOutput.sendMessage([144,inctState.ledPads[count],yellow]);
            }else{
              inControlOutput.sendMessage([144,x,ColorOff]);
            }
          }
        }
        count++;
      }
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
    for(var x=0; x<inctState.looperModeDeckValues.length; x++){
      inctState.looperModeDeckValues[x]=0;
      var baseNote=inctState.ledPads[0];
      var outputChannel=internals.noteOnChannelOne+inctState.padsOutputChannels[2]-1;
      rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+1, 0]);
      rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+3, 0]);
      rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+4, 0]);
      rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+0, 0]);
      rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+2, 0]);
    }
    for(var x=0; x<inctState.selectedFlexbeat.length; x++){
      inctState.selectedFlexbeat[x]=1;
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 127]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
    } 
    console.log("cleared lk pattern");
  }else if(scope.indexOf("lk")!=-1){
    for(var x=0; x<internals.loopMaxLength; x++){
      var tempStep=[];
      for(var y=0; y<internals.lkLoopData[globals.selectedPattern-1][x].length; y++){
        var recordedNoteChannel=internals.lkLoopData[globals.selectedPattern-1][x][y][0];
        var currentOutputChannels=[
          inctState.padsOutputChannels[inctState.padMode]+144-1,
          inctState.padsOutputChannels[inctState.padMode]+144-1-16 
        ]
        if(inctState.padMode==2 && inctState.looperModeBank!=1){ // clear synth instruments in looper mode
          currentOutputChannels=[ 0+144+inctState.mostRecentUpdatedSynthChan, 0+144-16+inctState.mostRecentUpdatedSynthChan];
        }
        if(!currentOutputChannels.includes(recordedNoteChannel)){ // retain messages from other channels
          tempStep.push(internals.lkLoopData[globals.selectedPattern-1][x][y]);
        }else if(inctState.padMode==0 && inctState.currentDrumBank!=Math.floor((internals.lkLoopData[globals.selectedPattern-1][x][y][1]-36)/16) ){ // retain drum hits from other banks
          tempStep.push(internals.lkLoopData[globals.selectedPattern-1][x][y]);
        }
      }
      internals.lkLoopData[globals.selectedPattern-1][x]=tempStep;
    }
    for(var x=0; x<inctState.drumPadState.length; x++){
      inctState.drumPadState[x]=false;
    }
    for(var x=0; x<inctState.looperModeDeckValues.length; x++){
      if(inctState.looperModeBank!=0 && inctState.padMode==2){ 
        inctState.looperModeDeckValues[x]=0;
        var baseNote=inctState.ledPads[0];
        var outputChannel=internals.noteOnChannelOne+inctState.padsOutputChannels[2]-1;
        rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+1, 0]);
        rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+3, 0]);
        rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+4, 0]);
        rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+0, 0]);
        rtpOutput.sendMessage([outputChannel, (8*x)+baseNote+2, 0]);
      }
    }
    for(var x=0; x<inctState.selectedFlexbeat.length; x++){
      inctState.selectedFlexbeat[x]=1;
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 127]);
      rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[1]-1, inctState.ledPadsDrumMap[inctState.ledPadsFlexbeatIndex.indexOf(1)]+(x*8), 0]);
    } 
    for(var x=0; x<inctState.drumRepeatBanks.length; x++){ // drum pad auto repeats
      for(var y=0; y<inctState.drumRepeatState[x].length; y++){
        inctState.drumRepeatState[x][y]=0;
        inctState.drumRepeatValues[x]=4;
      }
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
  for(var x=0; x<=4; x++){
    for(var y=0; y<=127; y++){
      if(inctState.midiNotesState[x][y]){
        rtpOutput.sendMessage([128+x+16,y,0]);
        rtpOutput.sendMessage([128+x,y,0]);
        inctState.midiNotesState[x][y]=false;
      }
    }
  }
  // for(var x=128; x<=131; x++){
  //   for(var y=0; y<=127; y++){
  //     rtpOutput.sendMessage([x+16,y,0]);
  //     rtpOutput.sendMessage([x,y,0]);
  //   }
  // }
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
      inctState.currentDrumBank=(inctState.currentDrumBank+(inctState.numberOfDrumBanks-1))%inctState.numberOfDrumBanks;
      for(var x=0; x<inctState.drumPadState.length; x++){
        inctState.drumPadState[x]=false;
      }
      console.log("current drum bank: "+inctState.currentDrumBank);
    }
    if(inctState.padMode==1){ // flexbeat bank
      inctState.flexbeatBank=(inctState.flexbeatBank+1)%inctState.numOfflexbeatBanks;
      console.log("flexbeat bank: "+inctState.flexbeatBank);
    }
    if(inctState.padMode==2){ // looper bank
      inctState.looperModeBank=(inctState.looperModeBank+1)%inctState.numOfLooperModeBanks;
      console.log("looper mode bank: "+inctState.looperModeBank);
    }
  }else if(message[0]==inctState.rightArrowButton[0] && message[1]==inctState.rightArrowButton[1] && message[2]==inctState.rightArrowButton[2]){ // right arrow button
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
      inctState.flexbeatBank=(inctState.flexbeatBank+(inctState.numOfflexbeatBanks-1))%inctState.numOfflexbeatBanks;
      console.log("flexbeat bank: "+inctState.flexbeatBank);
    }
    if(inctState.padMode==2){ // looper bank
      inctState.looperModeBank=(inctState.looperModeBank+(inctState.numOfLooperModeBanks-1))%inctState.numOfLooperModeBanks;
      console.log("looper mode bank: "+inctState.looperModeBank);
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
        clearLoop("lk.partial"); 
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
      if(inctState.drumRepeatBanks.includes(inctState.currentDrumBank)){ // drum bank with repeat controls
        sendToRtp=false;
        if( inctState.repeatControlsLedMap.includes(message[1])){ // controls half section
          inctState.drumRepeatValues[inctState.drumRepeatBanks.indexOf(inctState.currentDrumBank)]=inctState.repeatControlsLedMap.indexOf(message[1]);
        }else{ // normal drum pad half section
          inctState.drumPadState[inctState.ledPads.indexOf(message[1])]=(message[2]!=0);
          inctState.drumRepeatState[inctState.drumRepeatBanks.indexOf(inctState.currentDrumBank)][inctState.ledPads.indexOf(message[1])]=message[2];
        }
      }else{ // normal drum bank without repeat controls
        inctState.drumPadState[inctState.ledPads.indexOf(message[1])]=(message[2]!=0);
      }
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
      if(inctState.looperModeBank==0){ // first looper bank, looper controls
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
          globals.selectedLength=0;
          console.log("lk changed loop length: "+globals.selectedLength);
        }else if(message[1]==inctState.ledPads[5] && message[2]!=0){ // length button 
          globals.selectedLength=1;
          console.log("lk changed loop length: "+globals.selectedLength);
        }else if(message[1]==inctState.ledPads[6] && message[2]!=0){ // length button 
          globals.selectedLength=1;
          console.log("lk changed loop length: "+globals.selectedLength);
        }else if(message[1]==inctState.ledPads[7] && message[2]!=0){ // length button 
          globals.selectedLength=2;
          console.log("lk changed loop length: "+globals.selectedLength);
        }else if(message[1]==inctState.ledPads[12]){ // copy (shift) button  
          inctState.loopCopyShiftState = (message[2]!=0);
          console.log("lk shift copy: "+inctState.loopCopyShiftState);
        }        
      }
      if(inctState.looperModeBank==1){ // second looper bank, stutter
        syncWebsocket=true;
        syncLeds=true;
        shouldRecordMessage=false;
        sendToRtp=false;
        var padNum=inctState.repeatControlsLedPadsFullMap.indexOf(message[1]);
        var outputChannel=message[0]+inctState.padsOutputChannels[inctState.padMode]-1;
        var baseNote=inctState.ledPads[0];
        var deck=padNum>=8 ? 1 : 0;
        if(message[2]!=0){
          shouldRecordMessage=true;
          inctState.looperModeDeckValues[deck]=padNum%8;
          rtpOutput.sendMessage([outputChannel, (8*deck)+baseNote+1, stutterValues[padNum%8].steps]);
          rtpOutput.sendMessage([outputChannel, (8*deck)+baseNote+3, stutterValues[padNum%8].steplength]);
          rtpOutput.sendMessage([outputChannel, (8*deck)+baseNote+4, stutterValues[padNum%8].interval]);
          rtpOutput.sendMessage([outputChannel, (8*deck)+baseNote+0, stutterValues[padNum%8].mix]);
          rtpOutput.sendMessage([outputChannel, (8*deck)+baseNote+2, stutterValues[padNum%8].freeze]);
          transformedMessage=[outputChannel, padNum, 127];
        }
      }
    }
    if(inctState.padMode==3){ // slicer mode
      shouldRecordMessage=false;
      sendToRtp=false;
      syncLeds=true;
      if(message[2]!=0){
        let lengthModifier=internals.loopLengths[globals.selectedLength]/16;
        internals.cursor=(inctState.ledSlicerMap.indexOf(message[1])*lengthModifier);
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

lkInput.on('message', (deltaTime, message) => {
  if(globals.transportState=="rec" && message[0]!=248 && message[0]!=250 && message[0]!=252 ){
    recordMessage(message,"lkLoopData");
  }
  for(var x=0; x<3; x++){
    if(message[0]==144+x || message[0]==128+x){
      inctState.mostRecentUpdatedSynthChan=x;
    }
    if(message[0]==144+x){
      inctState.midiNotesState[x][message[1]]=true;
    }
    if(message[0]==128+x){
      inctState.midiNotesState[x][message[1]]=false;
    }
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
      var sendRtp=true;
      var playbacMessage=internals.lkLoopData[globals.selectedPattern-1][internals.cursor][x];
      var lkSynthNotes=[128, 129, 130, 144, 145, 146];
      if(lkSynthNotes.includes(playbacMessage[0])){ // update midi note state for lk synth
          if(playbacMessage[0]>=144){
            inctState.midiNotesState[playbacMessage[0]-144][playbacMessage[1]]=true;
          }else{
            inctState.midiNotesState[playbacMessage[0]-128][playbacMessage[1]]=false;
          } 
      }
      if(inctState.padMode==0 && (playbacMessage[0] == inctState.padsOutputChannels[inctState.padMode]+144-1 || playbacMessage[0] == inctState.padsOutputChannels[inctState.padMode]+144-1-16 )){ // drum mode
        let currentVelocity=playbacMessage[2]; // normal non-repeated drum hits
        let currentLedIndex=inctState.ledPadsDrumMap.indexOf(playbacMessage[1]-inctState.currentDrumBank*16);
        if(currentLedIndex!=-1 && currentVelocity!=0){
          inctState["drumPadState"][currentLedIndex]=true;
        }else if(currentLedIndex!=-1 && currentVelocity==0){
          inctState["drumPadState"][currentLedIndex]=false;
        }
        var playbackBank= Math.floor((playbacMessage[1]-36)/16); // drum repeat state updates
        if(currentLedIndex!=-1 && inctState.drumRepeatBanks.includes(playbackBank) && currentVelocity!=0){
          sendRtp=false;
          inctState["drumRepeatState"][inctState.drumRepeatBanks.indexOf(playbackBank)][currentLedIndex]=playbacMessage[2];
          console.log(playbacMessage[1]-inctState.currentDrumBank*16)
          if(inctState.repeatControlsLedMapTransformed.includes(playbacMessage[1]-inctState.currentDrumBank*16)){
            inctState.drumRepeatValues[inctState.drumRepeatBanks.indexOf(playbackBank)]=inctState.repeatControlsLedMapTransformed.indexOf(playbacMessage[1]-inctState.currentDrumBank*16);
          }
        }else if(currentLedIndex!=-1 && inctState.drumRepeatBanks.includes(playbackBank) && currentVelocity==0){
          sendRtp=false;
          inctState["drumRepeatState"][inctState.drumRepeatBanks.indexOf(playbackBank)][currentLedIndex]=playbacMessage[2];
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
      }else if((playbacMessage[0] == inctState.padsOutputChannels[2]+144-1 || playbacMessage[0] == inctState.padsOutputChannels[2]+144-1-16 )){ // looper mode second bank
        sendRtp=false;
        inctState.looperModeDeckValues[Math.floor(playbacMessage[1]/8)]=playbacMessage[1]%8;
        var baseNote=inctState.ledPads[0];
        var outputChannel=internals.noteOnChannelOne+inctState.padsOutputChannels[2]-1;
        rtpOutput.sendMessage([outputChannel, (8*Math.floor(playbacMessage[1]/8))+baseNote+1, stutterValues[playbacMessage[1]%8].steps]);
        rtpOutput.sendMessage([outputChannel, (8*Math.floor(playbacMessage[1]/8))+baseNote+3, stutterValues[playbacMessage[1]%8].steplength]);
        rtpOutput.sendMessage([outputChannel, (8*Math.floor(playbacMessage[1]/8))+baseNote+4, stutterValues[playbacMessage[1]%8].interval]);
        rtpOutput.sendMessage([outputChannel, (8*Math.floor(playbacMessage[1]/8))+baseNote+0, stutterValues[playbacMessage[1]%8].mix]);
        rtpOutput.sendMessage([outputChannel, (8*Math.floor(playbacMessage[1]/8))+baseNote+2, stutterValues[playbacMessage[1]%8].freeze]);
      }
      if(sendRtp){
        rtpOutput.sendMessage(playbacMessage);
      }
      syncLaunchkeyLEDS();
    }
    for(var x=0; x<inctState.drumRepeatBanks.length; x++){ // drum pad auto repeats
      for(var y=0; y<inctState.drumRepeatState[x].length; y++){
        if(inctState.drumRepeatState[x][y]!=0 && y>=8){ // dont send repeats for control half
          if(internals.cursor%inctState.drumRepeatIntervals[inctState.drumRepeatValues[x]]==0){
            rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[0]-1, inctState.ledPadsDrumMap[y]+(inctState.drumRepeatBanks[x]*16), inctState.drumRepeatState[x][y] ]);
            rtpOutput.sendMessage([internals.noteOnChannelOne+inctState.padsOutputChannels[0]-1-16, inctState.ledPadsDrumMap[y]+(inctState.drumRepeatBanks[x]*16), 0]);
          }
        }
      }
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
  res.send(JSON.stringify({"int":internals,"inctState":inctState, "stutterValues": stutterValues}));
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

app.get('/gui', function (req, res) {
  const options = {
      root: config.rootDir + "/"
  };

  const fileName = config.guiPath;
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
    fs.readFile(config.rootDir + config.presetsPath + parseInt(globals.presetName) + '.txt', 'utf8', (err, data) => {
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
    fs.writeFile(config.rootDir + config.presetsPath + parseInt(globals.presetName) + '.txt', fileContents, err => {
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
    dir = exec("sudo /usr/sbin/service " + config.systemdServiceName + " restart", function(err, stdout, stderr) {
      if (err) {
        console.log(err);
      }
      console.log(stdout);
    });
  }
}

function restartRtpMidi(midiInObject, midiOutObject, deviceString, portName, specialtyFunc){
  if(Date.now() - internals.last_rtp_reset_time < 60000){
    console.log("rtp midi was restarted less than 1 minute ago");
  }else{
    internals.last_rtp_reset_time = Date.now();
    dir = exec(config.rootDir + config.rtpResetScriptPath, function(err, stdout, stderr) {
      if (err) {
        console.log("error restarting rtp midi:");
        console.log(err);
      }else{
        console.log("restarted rtp midi");
        setTimeout(initMidiDevice, 5000, midiInObject, midiOutObject, deviceString, portName, specialtyFunc);
      }
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
      if(midiState[x][2].indexOf("rtpmid")!=-1){
        var rtpIsActive=false;
        for(var y=0; y<midiState[x][0].getPortCount(); y++){
          if(midiState[x][0].getPortName(y).indexOf(midiState[x][2])!=-1){
            rtpIsActive=true;
          }
        }
        if(rtpIsActive){
          console.log("rtpmid device ACTIVE");
        }else{
          console.log("rtpmid device INACTIVE");
          restartRtpMidi(midiState[x][0], midiState[x][1], midiState[x][2], midiState[x][3], midiState[x][4]);
        }

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
    // console.log("LISTED PORT: "+midiInObject.getPortName(x));
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
