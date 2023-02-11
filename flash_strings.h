#ifndef _FLASH_STRING_H
#define _FLASH_STRING_H

static const char FILE_MGR_HTML[] PROGMEM = "<!DOCTYPE html>\n\
<html><head> \n\
<meta charset='utf-8'> \n\
<style>\n\
  dt { text-align:left; font-size: 20px; width:500px;margin:0;border:0;padding:0} \n\
  .light { background-color:#eeeeee;}\n\
  .dark { background-color:#c0c0c0;}\n\
  div {width:500px; margin:0 auto; }\n\
  button {\n\
      border: 0;\n\
      border-radius: 1.5rem;\n\
      background-color: #1fa3ec;\n\
      color: #fff;\n\
      line-height: 2rem;\n\
      font-size: 1.5rem;\n\
      width: 32%\n\
  } \n\
</style></head> \n\
<body onload='list_file()'>\n\
<div> \n\
<h1 style='text-align:center;background-color: #1fa3ec; ' ><font color='white'>Esp File Manager</font></h1>\n\
<div id='lister'>\n\
  <dt class='light' onclick='choose(this)'> index.html------------322 bytes </dt>\n\
  <dt class='dark' onclick='choose(this)'> login.html------------22 bytes </dt>\n\
</div>\n\
<h2 id='shower'>selected file: </h2><br/>\n\
<div style='text-align:center;'>\n\
<button onclick='do_remove()'>remove</button>\n\
<button onclick='do_download()'>download</button>\n\
<button onclick='do_upload()'>upload</button>\n\
</div>\n\
</div>\n\
<script>\n\
  var selected_element;\n\
  var selected_file;\n\
  var http_path;\n\
  function choose(t) {\n\
    t.style.border='2px solid #1fa3ec';\n\
    var str = t.textContent;\n\
    selected_file=str.substring(0, str.indexOf('---'));\n\
    document.getElementById('shower').innerHTML = '<p>selected file: ' + selected_file + '</p>';\n\
    if (selected_element) selected_element.style.border='';\n\
    selected_element = t;\n\
  }\n\
     function do_upload() {\n\
      var input = document.createElement('input');\n\
      input.type = 'file';\n\
      input.click();\n\
      input.onchange = function(){\n\
        var file = input.files[0];\n\
          var form = new FormData();\n\
            form.append('file', file); \n\
            form.append('fileName', file.name);\n\
            var xhr = new XMLHttpRequest();\n\
            var action = 'upload';\n\
            xhr.open('POST', action);\n\
            xhr.send(form); \n\
            xhr.onreadystatechange = function(){\n\
            if(xhr.readyState==4 && xhr.status==200){\n\
              document.getElementById('lister').innerHTML = xhr.response;\n\
                  alert('file upload succeed!');\n\
              }\n\
          }\n\
      }\n\
      }\n\
  function do_download() {\n\
    if (!selected_file) {\n\
      alert('select a file first.');\n\
      return;\n\
    }\n\
    _url = http_path + 'download?file=' + selected_file;  \n\
    window.open(_url);\n\
  }\n\
  function do_remove() {\n\
    if (!selected_file) {\n\
      alert('select a file first.');\n\
      return;\n\
    }\n\
    var xmlhttp = new XMLHttpRequest(),\n\
        method = 'GET',\n\
        url = http_path + 'remove?file=' + selected_file; \n\
      console.log(url);\n\
      xmlhttp.open(method, url, true);\n\
    xmlhttp.onload = function () {// 处理取回的数据(在 xmlhttp.response 中找到)\n\
        document.getElementById('lister').innerHTML = xmlhttp.response;     \n\
        //console.log(xmlhttp.response);\n\
    };\n\
      xmlhttp.send();\n\
      selected_file = '';\n\
  }\n\
  function list_file() {\n\
    var my_url = window.location.href;\n\
      http_path = my_url.substring(0, my_url.lastIndexOf('/')) + '/';\n\
    var xmlhttp = new XMLHttpRequest(),\n\
        method = 'GET',\n\
        url = http_path + 'dir';\n\
      xmlhttp.open(method, url, true);\n\
    xmlhttp.onload = function () {\n\
        document.getElementById('lister').innerHTML = xmlhttp.response;\n\
    };\n\
      xmlhttp.send();\n\
  } \n\
</script>\n\
</body>\n\
</html>";


static const char PORTAL_HTML[] PROGMEM = "<!DOCTYPE html><html lang='en'>\n\
<head>\n\
    <meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no' />\n\
    <title>Config ESP</title>\n\
<style>\n\
body {\n\
    text-align: center;\n\
    font-family: verdana;    \n\
}\n\
input {width: 95%}\n\
button {\n\
    border: 0;\n\
    border-radius: .3rem;\n\
    background-color: #1fa3ec;\n\
    color: #fff;\n\
    line-height: 3rem;\n\
    font-size: 2rem;\n\
    width: 100%}\n\
div,input {\n\
    padding: 5px;\n\
    font-size: 1.7em;}\n\
div,body {\n\
    padding: 5px;}\n\
</style></head>\n\
<body>\n\
    <div style='text-align:center;display:inline-block;min-width:300px;'><br/>\n\
        <h1>Wifi Config</h1>\n\
        <form id='config' method='get' action='login'>\n\
            <input id='s' name='Wifi' length=32 placeholder='SSID'><br/><br/>\n\
            <input id='p' name='Password' length=64 type='password' placeholder='password'><br/><br/>\n\
            <br/><input id='h' name='Host' length=32 placeholder='Device Host Name'><br/><br/>\n\
            <button type='submit'>save</button>\n\
        </form><br/></div></body>\n\
</html>";


#endif
