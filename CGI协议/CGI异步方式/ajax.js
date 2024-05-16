function compute(arg) {
    let xHttp = new XMLHttpRequest();

    xHttp.onreadystatechange = function () {
        if (xHttp.readyState == 4 && xHttp.status == 200) {
            document.getElementById("results").innerHTML = xHttp.responseText;
            alert(xHttp.responseText);
        }
    }

    //发送异步请求构建,"/cgi‐bin/test_cgi.cgi"是指boa服务器中cgi‐bin目录下的test_cgi.cgi可执行文件
    let url = "/cgi-bin/test_cgi.cgi?"
    //获取用户输入的数字
    let data1 = document.getElementById("data1").value;
    let data2 = document.getElementById("data2").value;
    if (isNaN(data1) || isNaN(data2)) {
        alert("存在非数值类型，请重新输入");
        return;
    }
    url += data1;
    //判断用户需要的加法还是减法
    if (arg == '1') {
        url += "+";
    } else if (arg == '2') {
        url += "-";
    }
    url += data2;
    xHttp.open("GET", url, true);
    xHttp.send();

}