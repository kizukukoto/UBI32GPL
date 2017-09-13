function MM_swapImgRestore() { //v3.0
	var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
}

function MM_preloadImages() { //v3.0
	var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
	var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
	if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
}

function MM_findObj(n, d) { //v4.01
	var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
	d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
	if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
	for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
	if(!x && d.getElementById) x=d.getElementById(n); return x;
}

function MM_swapImage() { //v3.0
	var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
	if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
}

function get_file_size(which_size){
	var unit = new Array("Bytes", "KB", "MB", "GB", "TB", "PB", "EB");
	var count = 0;
	var str;
	
	while (which_size > 1024){
		which_size = which_size/1024;
		count++;
	}

	str = parseFloat(which_size).toFixed(2) + unit[count];

	return str;
}

function check_special_char(src_str){
	var dest_src = "";
	
	for (var i = 0; i < src_str.length; i++){
		var ch = src_str.charAt(i);
		
		if (ch == '>'){
			return false;
		}else if (ch == '<'){
			return false;
		}else if (ch == '/'){
			return false;
		}else if (ch == '\\'){
			return false;
		}else if (ch == '?'){
			return false;
		}else if (ch == ':'){
			return false;
		}else if (ch == '|'){
			return false;
		}else if (ch == '*'){
			return false;
		}else if (ch == '&'){
			return false;
		}else if (ch == '%'){
			return false;
		}else{
			return true;
		}
	}
}

function get_browser_height(){
	var viewportheight; 

	// the more standards compliant browsers (mozilla/netscape/opera/IE7) use window.innerWidth and window.innerHeight
	if (typeof window.innerWidth != 'undefined'){ 
      viewportheight = window.innerHeight ;

	// IE6 in standards compliant mode (i.e. with a valid doctype as the first line in the document) 
	}else if (typeof document.documentElement != 'undefined' 
			&& typeof document.documentElement.clientWidth != 'undefined'
			&& document.documentElement.clientWidth != 0){
			
       viewportheight = document.documentElement.clientHeight; 


	// older versions of IE 
	}else{ 
       viewportheight = document.getElementsByTagName('body')[0].clientHeight;
	}

	return viewportheight;
}

function get_browser_width(){
	var viewportwidth; 

	// the more standards compliant browsers (mozilla/netscape/opera/IE7) use window.innerWidth and window.innerHeight
	if (typeof window.innerWidth != 'undefined'){ 
      viewportwidth = window.innerWidth; 

	// IE6 in standards compliant mode (i.e. with a valid doctype as the first line in the document) 
	}else if (typeof document.documentElement != 'undefined' 
			&& typeof document.documentElement.clientWidth != 'undefined'
			&& document.documentElement.clientWidth != 0){
			
       viewportwidth = document.documentElement.clientWidth; 

	// older versions of IE 
	}else{ 
       viewportwidth = document.getElementsByTagName('body')[0].clientWidth;
	}

	return viewportwidth;
}

function get_week_day(index){
	var str = lang_obj.display('WDAY00' + index);
		
	return str;
}

function get_month(index){
	var str;

	if(index < 10){
		str = lang_obj.display('MON00' + index);
	}else{
		str = lang_obj.display('MON0' + index);
	}
		
	return str;
}

function get_digit_number(num){
	if (num <= 9){
		return "0" + num;
	}
	
	return num;
}

function ctime(mtime){
	var time_format = new Date(mtime*1000);
	var weekday = get_week_day(time_format.getUTCDay()+1);
	var month = get_month(time_format.getUTCMonth()+1);
	var date = get_digit_number(time_format.getUTCDate());
	var year = time_format.getUTCFullYear();
	var hour = get_digit_number(time_format.getUTCHours());
	var minute = get_digit_number(time_format.getUTCMinutes());
	var second = get_digit_number(time_format.getUTCSeconds());
	
	return weekday + " " + month + " " + date + " " + year + " " 
			+ hour + ":" + minute + ":" + second;
}

