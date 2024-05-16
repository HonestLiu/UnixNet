function compute(arg) {

    let xHttp = new XMLHttpRequest();

    xHttp.onreadystatechange = function () {
        if (xHttp.readyState == 4 && xHttp.status == 200) {
            document.getElementById("results").innerHTML = xHttp.responseText
        }
    }

    //Post请求不是明文传输，所以其不能加"?"
    let url = "/cgi-bin/post_cgi.cgi";
    //获取用户输入的数字
    let data = "";
    let data1 = document.getElementById("data1").value;
    let data2 = document.getElementById("data2").value;
    if (isNaN(data1) || isNaN(data2)) {
        alert("存在非数值类型，请重新输入");
        return;
    }
    data += data1;
    //判断用户需要的加法还是减法
    if (arg == '1') {
        data += "+";
    } else if (arg == '2') {
        data += "-";
    }
    data += data2;

    xHttp.open("POST", url, true);
    //数据需要使用send函数发送
    xHttp.send(data);

}
