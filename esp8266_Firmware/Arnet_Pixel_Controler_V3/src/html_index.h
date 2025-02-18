char html_main[] PROGMEM = R"=====(

<html>
    <head>
      <title>FlowMapper</title>
      <style>
        body{
          margin: 0px;
          background-color: grey;
          padding-bottom:75px;
        }
        label{
            display:block;
            position: absolute;
            width: 20%;
            padding-left:2%;
            border: 1px solid black;
            color:black;
            background-color:rgb(100, 100, 100);
            border-radius: 25px
        }
        #saveButton,input{
            display:block;
            position: absolute;
            height:100%;
            top:0%;
            width:74%;
            left:25%;
            border: 1px solid black;
            border-radius: 2px
        }
        #saveButton{
            background-color:rgb(60, 60, 60);
            height:175%;
            color:white;
            font-size:large;
        }
        input{
            background-color: white;
            padding-left: 10px;
        }
        .row{
            display: block;
            position:relative;
            /* background-color: blue; */
            width: 96%;
            height: 45px;
            line-height: 40px;
            margin-top:26px;
            left:2%;
            padding:0px;
            /* border: 1px solid black; */
        }
        body{ 
          padding-top: 0px;
        }
        .header {
            width:100%;
            display: grid;
            grid-template-columns: auto auto auto auto auto auto auto auto auto auto auto;
            background-color: black;
            padding: 0px;
            margin-bottom:35px;
            height:60px;
            z-index:10;
        
        }
        .header-item {
            color:white;
            background-color: black;
            padding: 20px;
            font-size: 19px;
            font-weight:bold;
            text-align: center;
            z-index:10;
            border-bottom:2px solid grey;
            border-top:2px solid grey;
         }
         @media only screen and (max-width:1000px){
            label{
                font-size: x-small;
            }
            .mobile{
                color: white;
            }
            .desktop{
                color: black;
            }
        }
        #dataPageGaugeTable {
            display: grid;
            grid-template-rows:16% auto auto;
            position:relative;
            background-color:rgb(100, 100, 100);
            width: 80%;
            height: 200px;
            margin-top:26px;
            left:10%;
            padding:0px;
            border: 1px solid black;
            border-radius:10px;
            text-align: center;
        }
        .dataPageGaugeTable-item{
            /* border: 1px solid black; */
            font-size: xx-large;
        }
        #testButton {
            display:block;
            position: relative;
            height:80px;
            top:30px;
            width:80%;
            left:10%;
            border: 1px solid black;
            border-radius: 2px;
            background-color:rgb(60, 60, 60);
            color:white;
            font-size:large;
        }
      </style>
    </head>
    <body>

        <div class="header">
      <div class="header-item">NimbusKit</div>
      <div class="header-item"></div>
      <div class="header-item"></div>  
      <div class="header-item"></div>
      <div class="header-item"></div>
      <div class="header-item"></div>
      <div class="header-item"></div>
      <div class="header-item"></div>
      <div class="header-item" onclick="goToPage('settings');">Settings</div>
      <div class="header-item" onclick="goToPage('data');">Data</div>
      <div class="header-item"></div>
    </div>

        <div id="dataPage">
            <div id="dataPageGaugeTable">
                <div class="dataPageGaugeTable-item"></div>  
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item" id="dataPageGauge">Test</div>
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item"></div>
                <div class="dataPageGaugeTable-item"></div>
            </div>
            <button id="testButton" onclick="socket.send('Socket_ABC123');">Test</button>
        </div>

        <script>

            var fields=[
                {
                    "name":"ssid",
                    "text":"SSID:",
                    "initVal":"---"
                },
                {
                    "name":"password",
                    "text":"Password:",
                    "initVal":"---"
                },
                {
                    "name":"universe",
                    "text":"Universe:",
                    "initVal":"---"
                },
                {
                    "name":"startChan",
                    "text":"Start Channel:",
                    "initVal":"---"
                },
                {
                    "name":"fixtureMode",
                    "text":"Fixture Mode:",
                    "initVal":"---"
                },
                {
                    "name":"oscAddressX",
                    "text":"OSC Address X:",
                    "initVal":"---"
                },
                {
                    "name":"oscAddressY",
                    "text":"OSC Address Y:",
                    "initVal":"---"
                },
                {
                    "name":"oscAddressZ",
                    "text":"OSC Address Z:",
                    "initVal":"---"
                },
                {
                    "name":"oscTargetIP",
                    "text":"OSC Target IP:",
                    "initVal":"---"
                },
                {
                    "name":"oscPort",
                    "text":"OSC Port:",
                    "initVal":"---"
                }
            ]

            window.onload = function(){
                initElements();
                initWebSocket();
            }

            function initElements(){

                for (var x in fields){
                    var row = document.createElement('div');
                    row.id=fields[x].name+'_row';
                    row.className='row';
                    var label = document.createElement('label');
                    row.appendChild(label);
                    label.for=fields[x].name;
                    label.innerHTML=fields[x].text;
                    var input = document.createElement('input');
                    row.appendChild(input);
                    input.type='text';
                    input.id=fields[x].name;
                    input.value=fields[x].initVal;
                    document.body.appendChild(row);
                }

                createSettingsButton();
                function createSettingsButton(){
                    var row = document.createElement('div');
                    row.id='saveButton_row'
                    row.className='row';
                    var button = document.createElement('button');
                    button.innerHTML='Save'
                    row.appendChild(button);
                    button.type='text';
                    button.id='saveButton';
                    button.onclick=function(){
                        makeUpdateSettingsPostCall();
                        console.log('clicked')
                        socket.send('clicked')
                    }
                    document.body.appendChild(row);
                }

                goToPage('data');
                makeGetSettingsCall();
            }

            function makeUpdateSettingsPostCall(){
                var payload={};
                for (var x in fields){
                  payload[fields[x].name]=document.getElementById(fields[x].name).value;
                }
                var urlString='http://'+location.host + '/updateSettings'
                var xhttp = new XMLHttpRequest(); 
                xhttp.onreadystatechange = function() {
                if(this.readyState == 4 && this.status == 200){
                    console.log(this.responseText);
                }
                };
                xhttp.open('POST', urlString, true);
                xhttp.setRequestHeader('Content-Type', 'application/json')
                xhttp.send(JSON.stringify(payload));
            }

            function makeGetSettingsCall(){
                var urlString='http://'+location.host + '/getSettings'
                var xhttp = new XMLHttpRequest(); 
                xhttp.onreadystatechange = function() {
                if(this.readyState == 4 && this.status == 200){
                    var getResponse=JSON.parse(this.response);
                    console.log(JSON.parse(this.response));
                    for (var x in fields){
                        document.getElementById(fields[x].name).value=getResponse[fields[x].name];
                    }
                }
                };
                xhttp.open('GET', urlString, true);
                xhttp.setRequestHeader('Content-Type', 'application/json')
                xhttp.send();
            }

            function makeResetCall(){
                var urlString='http://'+location.host + '/reset'
                var xhttp = new XMLHttpRequest(); 
                xhttp.onreadystatechange = function() {
                if(this.readyState == 4 && this.status == 200){
                    console.log(this.response);
                }
                };
                xhttp.open('GET', urlString, true);
                xhttp.setRequestHeader('Content-Type', 'application/json')
                xhttp.send();
            }

            function initWebSocket(){
                socket = new WebSocket("ws:/" + "/" + location.host + ":81");
                socket.onopen = function(e) {  console.log("[socket] socket.onopen "); };
                socket.onerror = function(e) {  console.log("[socket] socket.onerror ");console.log(e); };
                socket.onmessage = function(e) {  
                    console.log('Received data from websocket: '+e.data);
                    document.getElementById('dataPageGauge').innerHTML=e.data;
                    return true;
                };
            }

            function goToPage(pageName){
                if(pageName=="settings"){
                    for (var x in fields){
                        document.getElementById(fields[x].name+'_row').style.display='block';
                    }
                    document.getElementById('saveButton_row').style.display='block';
                    document.getElementById('dataPage').style.display='none';
                }
                if(pageName=="data"){
                    for (var x in fields){
                        document.getElementById(fields[x].name+'_row').style.display='none';
                    }
                    document.getElementById('saveButton_row').style.display='none';
                    document.getElementById('dataPage').style.display='block';
                }
            }

        </script>

    </body>
</html>
)=====";
