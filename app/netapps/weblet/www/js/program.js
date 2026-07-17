var programmes;
var totalpages = 0;
var currentpage = 0;
var curplayprog = -1;
var perpagenumber = window.screen.availHeight<1000 ? 15 : 20;
var curgroupid=0;
var curgroupname="ALL";

function filllist(sync) //sync need get curprog before filllist
{
    var e;
    var node;
    var start;
    var total;

    /* clear list */
    while (document.getElementById("programmes").childNodes.length)
        document.getElementById("programmes").removeChild(document.getElementById("programmes").firstChild);

    if(!sync)getcurprog();

    start = perpagenumber*currentpage;
    total = programmes.length-start;
    if (total>perpagenumber) total = perpagenumber;

    /* fill list */
    for (var i=start; i<start+total; ++i)
    {
        e = document.createElement("li");
        e.setAttribute("id", programmes[i].id);
        e.setAttribute("style", "word-break:break-all");
        e.onclick = switchprogramme;


        textNode = document.createTextNode((i+1) + '. ' + programmes[i].name.replace(/ /g,"\u00A0")+' ');
        e.appendChild(textNode);
        if(programmes[i].scramble==="Y"){
            addProgListIcon("money",e);
        }
        if(programmes[i].lock==="Y"){
            addProgListIcon("lock",e);
        }
        document.getElementById("programmes").appendChild(e);
        if(sync){
            if(programmes[i].id === curplayprog)e.style.backgroundColor="#e7c64e";
        }
    }
}

function addProgListIcon(iconName,parentNode)
{
    spanNode = document.createElement('span');
    imgNode = document.createElement('img');
    textNode = document.createTextNode(" ");
    imgsrc = "../img/" +iconName+ ".png"
    imgNode.setAttribute('src',imgsrc);
    spanNode.appendChild(imgNode);
    spanNode.setAttribute("style","vertical-align: middle; padding-bottom: 0.2")
    parentNode.appendChild(spanNode);
    parentNode.appendChild(textNode);

}

function flushlist()
{
    if (this.readyState!=4) return;
    if (this.status!=200)
    {
        alert(this.responseText);
        return;
    }

    // if this.responseText utf8 not correct, eval/parse would halt
    //var ret = eval("(" + this.responseText + ")").programmes;
    var data = JSON.parse(this.responseText);
    programmes = data.programmes;
    totalpages = Math.ceil(programmes.length/perpagenumber);
    if (!totalpages) totalpages = 1;
    currentpage = 0;
    filllist();
    document.getElementById("pageinfo").innerHTML = '1' +'/' +totalpages;
}

function showcurgroup()
{
    if (this.readyState!=4)return;
    if (this.status!=200)
    {
        alert(this.status);
        return;
    }
    var data = JSON.parse(this.responseText);
    curgroupid=data.id;
    curgroupname= data.name;

    document.getElementById("groupinfo").innerHTML = curgroupname;

}

function getcurgroup()
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = showcurgroup;

    xmlhttp.open("GET", "/getcurgroup", true);
    xmlhttp.send();
}

function fillcurproglist()
{
    if (this.readyState!=4)return;
    if (this.status!=200)
    {
        alert(this.status);
        return;
    }
    var data = JSON.parse(this.responseText);
    curplayprog = data.curprog[0].id;
    currentpage = Math.ceil((curplayprog+0.5)/perpagenumber)-1;
    filllist(true);
    document.getElementById("pageinfo").innerHTML = (currentpage+1) +'/' +totalpages;

    getcurgroup();
}

function flushcurproglist()
{
    if (this.readyState!=4) return;
    if (this.status!=200)
    {
        alert(this.responseText);
        return;
    }
    // if this.responseText utf8 not correct, eval/parse would halt
    //					var ret = eval("(" + this.responseText + ")").programmes;
    var data = JSON.parse(this.responseText);
    programmes = data.programmes;
    if(programmes.length)
    {
        totalpages = Math.ceil(programmes.length/perpagenumber);
        if (!totalpages) totalpages = 1;

        var xmlhttp;
        if (window.XMLHttpRequest)
            xmlhttp = new XMLHttpRequest();
        else
            xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

        xmlhttp.onreadystatechange = fillcurproglist;

        xmlhttp.open("GET", "/curprog", true);
        xmlhttp.send();
    }
    else
    {
        curplayprog = -1;
        currentpage = 0;
        totalpages=0;
        filllist(true);
        document.getElementById("pageinfo").innerHTML = (currentpage+1) +'/' +(totalpages+1);
    }
}

function getprogrammes()
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = flushcurproglist;

    xmlhttp.open("GET", "/programmes", true);
    xmlhttp.send();
}

function reflashlist()
{
    getprogrammes();
    document.getElementById('keyword').value='';
}

function getliststatus()
{
    var liststatus = "valid";
    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = function()
    {
        if (this.readyState!=4) return;
        if (this.status!=200)
        {
            alert(this.responseText);
            return;
        }
        liststatus = JSON.parse(this.responseText).curstatus[0].sta;
    }
    xmlhttp.open("GET", "/getliststatus", false);//sync
    xmlhttp.send();

    return liststatus;
}

function switchprogramme()
{
    var xmlhttp;
    var start;
    var total;
    var that=this;

    if(getliststatus() === "invalid")
    {
        reflashlist();
    }
    else
    {
        if (window.XMLHttpRequest)
            xmlhttp = new XMLHttpRequest();
        else
            xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");


        xmlhttp.onreadystatechange = function()
        {
            {
                if(xmlhttp.readyState!=4)
                    return;
                if(xmlhttp.status!=200)
                {
                    reflashlist();
                    return;
                }
            }

            //that.style.color = "darkblue";
            start = perpagenumber*currentpage;
            total = programmes.length-start;
            if (total>perpagenumber) total = perpagenumber;
            var h = document.getElementsByTagName("li");
            //重置颜色
            for(var k=0;k<total;k++)
            {
                //h[k].style.color="#4d4a42";
                h[k].style.backgroundColor="transparent";
            }
            //设置当前的样式
            //that.style.color="darkblue";
            that.style.backgroundColor="#e7c64e";
            curplayprog = that.id;

        };

        xmlhttp.open("POST", "/programme", true);
        xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
        xmlhttp.send("id="+this.id);
    }
}

function changegroup(dir)
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = function()
    {
        if (xmlhttp.readyState!=4)
            return;
        if(xmlhttp.status!=200)
        {
            return
        }

        getprogrammes();

    }

    xmlhttp.open("POST", "/changegroup", true);
    xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
    xmlhttp.send("dir="+dir);
}

function changepage(id)
{
    if (totalpages<2) return;

    if (id == "next")
        currentpage = (currentpage+1)%totalpages;
    else if (!currentpage)
        currentpage = totalpages-1;
    else
        --currentpage;

    filllist();
    document.getElementById("pageinfo").innerHTML = (currentpage+1) + '/' + totalpages;
}

function search()
{
    var keyword = document.getElementById("keyword").value;
    var xmlhttp;

    if (keyword.length > 128) return;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = flushlist;

    xmlhttp.open("POST", "/finder", true);
    xmlhttp.send("keyword="+encodeURIComponent(keyword));
}

function getcurprogchange()
{
    if (this.readyState!=4) return;
    if (this.status!=200)
    {
        alert(this.responseText);
        return;
    }

    var data = JSON.parse(this.responseText);
    curplayprog = data.curprog[0].id;
    var start;
    var total;

    start = perpagenumber*currentpage;
    total = programmes.length-start;
    if (total>perpagenumber) total = perpagenumber;
    var h = document.getElementsByTagName("li");
    for (var i=start; i<start+total; ++i)
    {
        if(programmes[i].id == curplayprog)
        {
            h[i-start].style.backgroundColor="#e7c64e";
            break;
        }
    }
}
function getcurprog()
{
    var xmlhttp;

    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest();
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

    xmlhttp.onreadystatechange = getcurprogchange;

    xmlhttp.open("GET", "/curprog", true);
    xmlhttp.send();
}

function remote(key)
{
	var xmlhttp;

	if (window.XMLHttpRequest)
		xmlhttp = new XMLHttpRequest();
	else
		xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

	xmlhttp.onreadystatechange = function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status!=200)
		{
			alert(xmlhttp.responseText);
		}
	}

	xmlhttp.open("POST", "/remote", true);
	xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
	xmlhttp.send("key="+key);
}
