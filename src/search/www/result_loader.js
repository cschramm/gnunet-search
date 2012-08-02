function load_results(query, offset) {
	if (!offset) {
		offset = 0;
	}
	xmlHttp = new XMLHttpRequest();
	xmlHttp.open('GET', 'results?q='+encodeURIComponent(query)+'&o='+encodeURIComponent(offset), true);
	xmlHttp.onreadystatechange = function () {
		if (xmlHttp.readyState == 4) {
			alert(xmlHttp.responseText);
		}
	};
	xmlHttp.send(null);
}
