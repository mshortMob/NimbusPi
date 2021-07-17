const midi = require('midi');
const circuitInput = new midi.Input();
const rolandInput = new midi.Input();
const rolandOutput = new midi.Output();

const express = require('express');
const bodyParser = require('body-parser');
const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const port = 3000;

init();
function init(){
  circuitPort="None";
  rolandPort="None";
  cursor=0;
  loopData=[];
  state="play";
  loopMaxLength=96*4;  // in tickts, 6 ticks == 1 quater note == 1 beat
  loopLengths=[96*1, 96*2, 96*4];
  selectedLength=0;
  selectedPattern=1;
  quantizeEnabled=false;
  for(var y=0; y<5; y++){
    var temp=[];
    for(var x=0; x<loopMaxLength; x++){
      temp.push([]);
    }
    loopData.push(temp);
  }
  initMidiConnections();
}

function initMidiConnections(){
  for(var x=0; x<circuitInput.getPortCount(); x++){
    if(circuitInput.getPortName(x).indexOf("Circuit")!=-1){
      circuitPort=x;
      console.log("Found circuit on port: "+x);
    }
    if(circuitInput.getPortName(x).indexOf("Boutiq")!=-1){
      rolandPort=x;
      console.log("Found roland on port: "+x);
    }
  }
  if(circuitPort!="None" && rolandPort!="None"){
    circuitInput.openPort(circuitPort);
    circuitInput.ignoreTypes(true,false,true);
    rolandInput.openPort(rolandPort);
    rolandOutput.openPort(rolandPort);
  }else{
    console.log("A midi device is missing, exiting");
    process.exit(1);
  }
}

circuitInput.on('message', (deltaTime, message) => {
  if(message[0]==248){
    // console.log(cursor);
    cursor=(cursor+1)%loopLengths[selectedLength];
  }
  if(message[0]==250){
    cursor=0;
    state="play";
    console.log("circuit play pressed");
  }
  if(message[0]==252){
    state="stop";
    killAllNotes();
    console.log("circuit stop pressed");
  }
  if(state=="play" || state=="rec"){
    for(var x=0; x<loopData[selectedPattern-1][cursor].length; x++){
      rolandOutput.sendMessage(loopData[selectedPattern-1][cursor][x]);
    }
  }
});

function killAllNotes(){
  for(var x=48; x<=72; x++){
    // rolandOutput.sendMessage([130,parseInt(x, 16),0]);
    rolandOutput.sendMessage([130,x,0]);
  }
}

rolandInput.on('message', (deltaTime, message) => {
  if(state=="rec"){
    if(quantizeEnabled==false){
      if(selectedLength==0){
        loopData[selectedPattern-1][cursor].push(message);
        loopData[selectedPattern-1][cursor+96*1].push(message);
        loopData[selectedPattern-1][cursor+96*2].push(message);
        loopData[selectedPattern-1][cursor+96*3].push(message);
      }else if(selectedLength==1){
        loopData[selectedPattern-1][cursor].push(message);
        loopData[selectedPattern-1][cursor+96*2].push(message);
      }else{
        loopData[selectedPattern-1][cursor].push(message);
      }
    }else{
      if(selectedLength==0){
        loopData[selectedPattern-1][cursor-(cursor%6)].push(message);
        loopData[selectedPattern-1][cursor-(cursor%6)+96*1].push(message);
        loopData[selectedPattern-1][cursor-(cursor%6)+96*2].push(message);
        loopData[selectedPattern-1][cursor-(cursor%6)+96*3].push(message);
      }else if(selectedLength==1){   
        loopData[selectedPattern-1][cursor-(cursor%6)].push(message);
        loopData[selectedPattern-1][cursor-(cursor%6)+96*2].push(message);
      }else{
        loopData[selectedPattern-1][cursor-(cursor%6)].push(message);
      }
    }
  }
});

function clearLoop(){
  for(var x=0; x<loopMaxLength; x++){
    loopData[selectedPattern-1][x]=[];
  }
  killAllNotes();
}

app.get('/play', (req, res) => {
  res.send('receiced play cmd')
  console.log('receiced play cmd');
  state="play";
})

app.get('/rec', (req, res) => {
  res.send('receiced rec cmd')
  console.log('receiced rec cmd');
  state="rec";
  // clearLoop();
})

app.get('/stop', (req, res) => {
  res.send('receiced stop cmd')
  console.log('receiced stop cmd');
  state="stop";
  killAllNotes();
})

app.get('/clear', (req, res) => {
  res.send('receiced clear cmd')
  console.log('receiced clear cmd');
  clearLoop();
})

app.get('/length/:selectedLength', (req, res) => {
  res.send('receiced length cmd: '+req.params.selectedLength);
  selectedLength=req.params.selectedLength;
  console.log('receiced length cmd:'+selectedLength);
})

app.get('/pattern/:patternNumber', (req, res) => {
  res.send(req.params.patternNumber);
  // console.log(loopData[selectedPattern-1]);
  console.log("change to pattern:"+req.params.patternNumber);
  killAllNotes();
  selectedPattern=parseInt(req.params.patternNumber);
})

app.get('/saveToDisk', (req, res) => {
  res.send(JSON.stringify(loopData));
  console.log("sending pattern contents to from end")
})

app.post('/recallFromDisk', (req, res) => {
  res.send("updating loop contents");
  console.log("loading pattern contents sent from front end");
  // console.log(req.body);
  loopData=req.body;
})

app.get('/quantize/:enable', (req, res) => {
  quantizeEnabled=(req.params.enable === 'true');
  console.log("set quantanization: "+req.params.enable);
  res.send("set quantanization: "+req.params.enable);
})

app.get('/killAllNotes', (req, res) => {
  killAllNotes();
  res.send("killAllNotes");
  console.log("killAllNotes");
})

app.get('/test', (req, res) => {
  res.send(JSON.stringify(loopData));
  saveState();
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})



