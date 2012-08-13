function load_results(query, offset) {
	if (!offset) {
		offset = 0;
	}
	xmlHttp = new XMLHttpRequest();
	xmlHttp.open('GET', 'results?q='+encodeURIComponent(query)+'&o='+encodeURIComponent(offset), true);
	xmlHttp.onreadystatechange = function () {
		if (xmlHttp.readyState == 4) {
			results = JSON.parse(xmlHttp.responseText);
			for (i in results) {
				var li = document.createElement('li');
				var a = document.createElement('a');
				a.href = results[i];
				var text = document.createTextNode(results[i]);
				a.appendChild(text);
				li.appendChild(a);
				document.getElementsByTagName('ul')[0].appendChild(li);
				offset++;
			}
			window.setTimeout(function () {
				load_results(query, offset);
			}, 1000);
		}
	};
	xmlHttp.send(null);
}
