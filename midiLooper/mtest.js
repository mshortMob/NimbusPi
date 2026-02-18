const fs = require('fs');
const { exec } = require('child_process');
const midi = require('midi');
const {WebSocketServer} = require('ws');
const wss = new WebSocketServer({ port: 3003 });
const circuitInput = new midi.Input();
const circuitOutput = new midi.Output();
const rolandInput = new midi.Input();
const rolandOutput = new midi.Output();
const lkInput = new midi.Input();
const lkOutput = new midi.Output();
const inControlInput = new midi.Input();
const inControlOutput = new midi.Output();
const rtpInput = new midi.Input();
const rtpOutput = new midi.Output();
const express = require('express');
const bodyParser = require('body-parser');
const { channel } = require('diagnostics_channel');
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
    loopMaxLength: 96*4,  // in tickts, 6 ticks == 1 quater note == 1 beat
    loopLengths: [96*1, 96*2, 96*4],
    noteOnChannelOne: 144,
    noteOffChannelOne: 128
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

  inControlState={
    padMode: 0,
    numberOfPadModes: 3,
    padsOutputChannels: [14,15,16],
    ledPads: [96,97,98,99,112,113,114,115,116,117,118,119,100,101,102,103],
    ledPadsDrumMap: [36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51],
    upCircleButton: [144,104,127],
    downCircleButton: [144,120,127],
    upArrowButton: [176,104,127],
    downArrowButton: [176,105,127],
    leftArrowButton: [144,106,127],
    rightArrowButton: [144,107,127],
    knobs: [ [176,21], [176,22], [176,23], [176,24], [176,25], [176,26], [176,27], [176,28] ],
    knobsOutputMap: [
      [
        {
          control: 21,
          inputRange: [0,127],
          outputRange: [0,127]
        },
        {
          control: 29,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 22,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 23,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 24,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 25,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 26,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 27,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ],
      [
        {
          control: 28,
          inputRange: [0,127],
          outputRange: [0,127]
        }
      ]
    ]
  }

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

  initLoopData();
  initCircuitProgramLoopData();
  initMidiConnections();

}

function initMidiConnections(){
  console.log(" ");
  console.log("Listing all MIDI Ports:");
  console.log("--------------------------");
  for(var x=0; x<circuitInput.getPortCount(); x++){
    console.log("Port "+x+": "+circuitInput.getPortName(x));
  }
  console.log("--------------------------");
  console.log(" ");
  for(var x=0; x<circuitInput.getPortCount(); x++){
    if(circuitInput.getPortName(x).indexOf("Circuit")!=-1){
      internals.circuitPort=x;
      console.log("Found circuit on port: "+x);
    }
    if(circuitInput.getPortName(x).indexOf("Boutiq")!=-1){
      internals.rolandPort=x;
      console.log("Found roland on port: "+x);
    }
    if(circuitInput.getPortName(x).indexOf("Launchkey")!=-1 && internals.lkPort=="None"){
      internals.lkPort=x;
      console.log("Found launcheky on port: "+x);
    }
    if(circuitInput.getPortName(x).indexOf("InContro")!=-1 && internals.inControlPort=="None"){
      internals.inControlPort=x;
      console.log("Found inControl on port: "+x);
    }
    if(circuitInput.getPortName(x).indexOf("rtpmidid:mpc-one/MPC-rtpmidi mpc-one Network")!=-1 && internals.rtpPort=="None"){
      internals.rtpPort=x;
      console.log("Found rtpMidi on port: "+x);
    }
  }
  if(internals.lkPort!="None"){
    lkInput.openPort(internals.lkPort);
    lkOutput.openPort(internals.lkPort);
  }
  if(internals.inControlPort!="None"){
    inControlInput.openPort(internals.inControlPort);
    inControlOutput.openPort(internals.inControlPort);
    for(var x=0;x<200;x++){
      inControlOutput.sendMessage([144,x,127]);  
    }
  }
  if(internals.circuitPort!="None"){
    circuitInput.openPort(internals.circuitPort);
    circuitInput.ignoreTypes(true,false,true);
    circuitOutput.openPort(internals.circuitPort);
  }
  if(internals.rolandPort!="None"){
    rolandInput.openPort(internals.rolandPort);
    rolandOutput.openPort(internals.rolandPort);
  }
  if(internals.rtpPort!="None"){
    rtpInput.openPort(internals.rtpPort);
    rtpOutput.openPort(internals.rtpPort);
  }

  // console.log("A midi device is missing, exiting");
  // process.exit(1);
}

function clearLoop(scope){
  if(scope=="roland" || scope == "all"){
    for(var x=0; x<internals.loopMaxLength; x++){
      internals.loopData[globals.selectedPattern-1][x]=[];
    }
    killAllNotes();
  }
  if(scope=="circuit" || scope == "all"){
    for(var x=0; x<internals.loopMaxLength; x++){
      internals.circuitProgramLoopData[globals.selectedPattern-1][x]=[];
    }
  }
}

function killAllNotes(){
  console.log("killing all notes")
  for(var x=48; x<=72; x++){
    // rolandOutput.sendMessage([130,parseInt(x, 16),0]);
    rolandOutput.sendMessage([130,x,0]);
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

function syncLaunchkeyLEDS(){
  if(inControlState.padMode==0){ // padMode 0=orange, 1=red, 2=green
    playCursorOnLEDS(127, 127);
  }else if(inControlState.padMode==1){
    playCursorOnLEDS(9, 9);
  }else{
    playCursorOnLEDS(100, 100);
  }
  
  function playCursorOnLEDS(ColorOn, ColorOff){
    let selectedStep=Math.floor(internals.cursor/6);
    let count=0;
    for(var x of inControlState.ledPads){
      if(count==selectedStep && globals.transportState!="stop"){
        inControlOutput.sendMessage([144,x,ColorOn]);
      }else{
        inControlOutput.sendMessage([144,x,ColorOff]);
      }
      count++;
    }
    inControlOutput.sendMessage([inControlState.upCircleButton[0],inControlState.upCircleButton[1],ColorOff]);
    inControlOutput.sendMessage([inControlState.downCircleButton[0],inControlState.downCircleButton[1],ColorOff]);

  }
}

lkInput.on('message', (deltaTime, message) => {
  rtpOutput.sendMessage(message);
  console.log("lkInput: "+ message );
});

inControlInput.on('message', (deltaTime, message) => {
  console.log("inControlInput: "+ message);
  let sendToRtp=true;
  let syncLeds=false;
  let transformedMessage=message;
  if(message[0]==inControlState.upArrowButton[0] && message[1]==inControlState.upArrowButton[1] && message[2]==inControlState.upArrowButton[2]){ // up arrow button
    inControlState.padMode=(inControlState.padMode+1)%inControlState.numberOfPadModes;
    sendToRtp=false;
    syncLeds=true;
    console.log("pad mode: "+inControlState.padMode);
  }else if(message[0]==inControlState.downArrowButton[0] && message[1]==inControlState.downArrowButton[1] && message[2]==inControlState.downArrowButton[2]){ // down arrow button
    inControlState.padMode=(inControlState.padMode+2)%inControlState.numberOfPadModes;
    sendToRtp=false;
    syncLeds=true;
    console.log("pad mode: "+inControlState.padMode); 
  }else if(inControlState.ledPads.includes(message[1])){ // pad buttons
    sendToRtp=true;
    transformedMessage=[message[0]+inControlState.padsOutputChannels[inControlState.padMode]-1,inControlState.ledPadsDrumMap[inControlState.ledPads.indexOf(message[1])],message[2]];
  }else if(inControlState.knobs.includes([message[0],message[1]])){ // knobs (broken)
    sendToRtp=true;
    transformedMessage=[message[0], message[1], message[2]];
  }else{
    sendToRtp=true;
  }
  if(sendToRtp){
    rtpOutput.sendMessage(transformedMessage);
    console.log("inControlInput transformedMessage: "+transformedMessage);
  }
  if(syncLeds){
    syncLaunchkeyLEDS();
  }
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
  }
});

rolandInput.on('message', (deltaTime, message) => {
  recordMessage(message, "loopData");
});

app.get('/action', (req, res) => {
  var action=req.query.value;
  console.log('received '+action+' cmd');
  if(action=="clear"){
    clearLoop("all");
  }
  if(action=="clearRoland"){
    clearLoop("roland");
  }
  if(action=="clearCircuit"){
    clearLoop("circuit");
  }
  if(action=="killnotes"){
    killAllNotes();
  }
  if(action=="recall"){
    fs.readFile('/root/NimbusPi/midiExperiements/presets'+parseInt(globals.presetName)+'.txt', 'utf8', (err, data) => {
      if (err) {
        console.error(err);
        return;
      }
      var tempData=JSON.parse(data);
      internals.loopData=tempData.roland;
      internals.circuitProgramLoopData=tempData.circuit;
      console.log('recalled saved presets!');
    });
  }
  if(action=="save"){
    var fileContents=JSON.stringify({ "roland": internals.loopData, "circuit": internals.circuitProgramLoopData});
    fs.writeFile('/root/NimbusPi/midiExperiements/presets'+parseInt(globals.presetName)+'.txt', fileContents, err => {
      if (err) {
        console.error(err);
      }
      console.log('saved preset data!');
    });
  }
  if(action=="copy"){
    internals.loopData[parseInt(req.query.copyTarget-1)]=JSON.parse(JSON.stringify(internals.loopData[globals.selectedPattern-1]));
    internals.circuitProgramLoopData[parseInt(req.query.copyTarget-1)]=JSON.parse(JSON.stringify(internals.circuitProgramLoopData[globals.selectedPattern-1]));
  }
  if(action=="reload"){
    dir = exec("sudo /usr/sbin/service mtest restart", function(err, stdout, stderr) {
      if (err) {
        console.log(err);
      }
      console.log(stdout);
    });
  }
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
  res.send(JSON.stringify(internals));
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

app.get('/gui', function (req, res) {
  const options = {
      root: "/root/NimbusPi/midiLooper/"
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
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(globals));
    }
  });
}
