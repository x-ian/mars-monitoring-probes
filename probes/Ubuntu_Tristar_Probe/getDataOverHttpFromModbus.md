* Tristar controller has a Javascript Web-UI to be used by the browser
* The Modbus communication usually happens via TCP
* Modbus is pretty low-level and values need to be converted with different scale factors and formulas
* TriStar exposes Modbus via HTTP with a CGI endpoint (like [[http://192.168.2.10/MBCSV.cgi?ID=...]])
* The Web-UI already translates the cryptic Modbus messages into something human-readable
* Scripting of the Web-UI isn't straight forward as all values are loaded/updated through Javascript and AJAX calls, therefore simple wget/curl HTTP calls do not contain the readings
* Setting up a dedicated HTML file replicating some/most of the TriStar Web-UI Javascript logic doesn't work because of the default 'same origin policy' in browsers
* Using phantomjs environment allows to script the Javascript communication while also ignoring the 'same origin policy'
* each value is stored in its own result file
* phantomjs --web-security=false ../getDataOverHttpFromModbus.js
* Using the HTML file getDataOverHttpFromModbus.html can help to test / debug the Javascript up to the point of Modbus CGI invocation (which from here then fails because of the 'same origin policy')
* Tristar IP hardcoded in the JS as well as the exact model and modbus number