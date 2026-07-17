
function content_get()
{
    if (this.readyState!=4) return;
    if (this.status!=200)
    {
        document.getElementById("keyword").value = "";
        return;
    }
    var data = JSON.parse(this.responseText);
    var input;
    input = data.keyedit;
    document.getElementById("keyword").value = input[0].content;
}

function reflashinput()
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = content_get;

    xmlhttp.open("GET", "/kb_update", true);
    xmlhttp.send();
}

function kb_need_sync()
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = function()
    {
        if (this.readyState!=4) return;
        if (this.status!=200) return;

        var data = JSON.parse(this.responseText);
        var input;
        input = data.keyedit;
        if (input[0].content == "invalid")
            kb_need_sync();
        else
            document.getElementById("keyword").value = input[0].content;

    };
    xmlhttp.open("GET", "/kb_edit_sync", true);
    xmlhttp.send();
}

function push()
{
    var keyword = document.getElementById("keyword").value;
    var xmlhttp;

    if (keyword.length > 128) return;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.open("POST", "/pushedit", true);
    xmlhttp.send("edit="+encodeURIComponent(keyword));
}

function keyboardremote(key)
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    if(key == "blue" || key == "ok")
    {
        xmlhttp.onreadystatechange = function()
        {
            if (this.readyState!=4) return;
            if (this.status!=200) return;
            if (xmlhttp.readyState==4 && xmlhttp.status!=200)
            {
                alert(xmlhttp.responseText);
                return;
            }
            kb_need_sync();
        };
    }
    else
    {
        xmlhttp.onreadystatechange = function()
        {
            if (this.readyState!=4) return;
            if (this.status!=200) return;
            if (xmlhttp.readyState==4 && xmlhttp.status!=200)
            {
                alert(xmlhttp.responseText);
                return;
            }
            reflashinput();
        };
    }
    xmlhttp.open("POST", "/remote", true);
    xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
    xmlhttp.send("key="+key);
}

function confirm()
{
    var xmlhttp;
	var keyword = document.getElementById("keyword").value;

    if (keyword.length > 128) return;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.open("POST", "/confirm", true);
    xmlhttp.send("edit="+encodeURIComponent(keyword));
}
