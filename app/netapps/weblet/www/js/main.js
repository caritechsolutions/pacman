
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

		function getprogrammes()
		{
			var xmlhttp;

			if (window.XMLHttpRequest)
				xmlhttp = new XMLHttpRequest();
			else
				xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");

			xmlhttp.onreadystatechange = function()
			{
				if (xmlhttp.readyState==4 && xmlhttp.status==200)
				{
// if xmlhttp.responseText utf8 not correct, eval/parse would halt
//					var ret = eval("(" + xmlhttp.responseText + ")").programmes;
					var data = JSON.parse(xmlhttp.responseText);
					var ret = data.programmes;
					var e;
					var node;

					/* clear list */
					while (document.getElementById("programmes").childNodes.length)
						document.getElementById("programmes").removeChild(document.getElementById("programmes").firstChild);

					/* fill list */
					for (var i=0; i<ret.length; ++i)
					{
						e = document.createElement("li");
						node = document.createTextNode(ret[i].name);
						e.setAttribute("id", ret[i].id);
						e.appendChild(node);
						
						e.onclick = switchprogramme;
						document.getElementById("programmes").appendChild(e);
					}
				}
			}

			xmlhttp.open("GET", "/programmes", true);
			xmlhttp.send();
		}

		function switchprogramme()
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

			xmlhttp.open("POST", "/programme", true);
			xmlhttp.setRequestHeader("Content-type","application/x-www-form-urlencoded");
			xmlhttp.send("id="+this.id);
		}

