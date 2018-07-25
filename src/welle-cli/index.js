var channels = [ "5A", "5B", "5C", "5D", "6A", "6B", "6C", "6D", "7A", "7B",
"7C", "7D", "8A", "8B", "8C", "8D", "9A", "9B", "9C", "9D", "10A", "10B",
"10C", "10D", "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A",
"13B", "13C", "13D", "13E", "13F"];

var png_cross =
"iVBORw0KGgoAAAANSUhEUgAAAUAAAADwCAAAAABURuK3AAAFA0lEQVR42u2dy44dIQxEu1D+/5edRaRIiRKpu/Gjii5vRncxd8yhzIABg7hsO7aMwAAN0AAN0GaABmiABmgzQAM0QAO0GaABGqAB2gzQAA3QAG0GaIAGaIA2AzRAAzRAmwEaoAEaoM0ADdAADdBmgEn2480v4dcP35C4rgvxFp8RvgSIPz59HuHa4/f3RyvwIT8dEVaN2wkAFQjWjdsPAf47YkMHX7q3K99Bcn7J3uZMpAEdfskEV5WXvPxIl3KsIkQx0yXT1aTduqS8JezTjHkg64wGHV4uvT7nioenAEOFYJcfjxUoQrDNi+chLEEQbQM1oqJ7g1t/4wBviCyIwzdmQ/ieC/gIv5cKvAUoKPEN5wPpCbZP9BF1fT1AsH+hhBAKF8aF5hZAMhGO9CdC0GkmV3YB0ohwqicRsq5zOJEAkEGEc2nKjHzg/LpkMM2LaAmgUg2OjiEI+UbMjsFZAMdEOP0vDKHdlPEpAEK5NQQzqFSA3XHMMANFXLIEKfaoEc1Bldcujj1+tA9LSS1jyWKgIMXYQZAmCwTJuQVTEq0mSVYrQqZMOKrSZIVtpDohhrI8WZkIuU7YQW2Rz7aRhcpUWYEI+TYCRxN1V4jjK75wnZ2qZjwVhup0WV6bOQ/VoTxflhXGpIcS0ZAxS2k56/UAiGza0l6vQE/SbDOOic8U95Q92VQY85lsdKXN3kPmPtLeVXjnvQbJrwSA/fgZ++U8kJ+ApL/ciN7k2dNQ578ciu7szxOC9DeiBgA+EKECvgGAt8Fo8JsAeEuEIvhmAGYcWKUpLTBSwTLO4TejwG0REpW2mKqhGofwG1Pghgi5asWB+Wo5P77hMsihz29WgY81SFjpcbgQd6jzG69kHuL8pkP4SRhzVmoFg1vQ5ccBkLyQD/UYeFeBtDVaGRR4E05YgXviggFucoEBblKhrBMstZQjHAedjREG+DIiuRAuOX5kI+GcAnc4eE9kU0f4vAL3CcSnFdhw6vxkBd7Cp5Kg6Qd4m4xPZ+1Gr4QIqQ9YKoiwGeBjIvxl+4OaH38cdwKsuefwkYs276ORfErYpsCNwYxagyx35WRvayIE+DEj5LgvHAlDwLlX/rPU89GiE4kNpxThfNWOmPoyDYDJTeYTYTHA9OuqdASXFj++dclo9bZg+lo+gGUNpQrjuQqWhzwnplhDlQrhUBXfhpecmhAuUX40/44rFNhUq4RDg0uWH4kGIf1GynEPsgw87zuNEOpPzEyPhNCPqEOeRbvmKoWNirDxWTS9tEXvNGayUt3+pt64AscfGJz6+zjlhaixpynjDH6X7BPhTJmlCRHuA6R6IWpgIn/QC2Uj7iCO4tcfxwglbwl9QpwkvwGCGwB5j+x1EnwPkPqNij7fcMb7gnMI3wLkfySgKY4RZ/JrE+ErgCpV2jtEuM7l17PviTgyfPs0iOwzomSFccqjZbWHDVkYA80KhBS/ehGuo/V3U4Q0AEkLnUYUEvxxPL5frpXta662bmaOY8wD5MZX6OD6gvwqCa4MPwTwlXm5vhC+lXGy9nGp8KtBuLYFJ8SvwtvdfKAWvv/NWaIZ4G8nBPH9k2A/QG1DYlyvD/JLHcU/qcDMYfyrANOG8e8CpFoLG6DNAA3QAA3QZoAGaIAGaDNAAzRAA7QZoAEaoAHaDNAADdAAbQZogAZogDYDNEADNECbARqgARqgzQAN0AAN0GaABmiA4vYT0KCGE2bgCksAAAAASUVORK5CYII=";

var png_alert = "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAk1BMVEWgAAD/AAD/////+/v/+Pj/2tr/T0//1NT/5eX/mJj/pKT/8/P/7Oz/y8v/4eH/Dg7/19f/R0f/vb3/Xl7/eHj/Njb/6+v/sbH/fHz/iYn/QUH/cXH/nZ3/HBz/JSX/bGz/jo7/xMT/KSn/ra3/Zmb/PT3/sLD/hIT/YGD/hob/VVX/Fhb/t7f/yMj/TU3/OTn/oqILUJpfAAAAAXRSTlMAQObYZgAAAFVJREFUOMuV0EEOACEIA0Dn/5/em8kqWumxTAJhjGZI8yAEIQDugiASIIgE5uAg/IE2sAJNYAdaQAU0gBqonlwCxzyCtdlEAlKxbnUH+5lCMnh51DUfEC4CA1JxhXIAAAAASUVORK5CYII="

window.onload = function() {
    var ch = document.getElementById("channelselector");

    for (c in channels) {
        var opt = document.createElement("option");
        opt.text = channels[c];
        ch.options.add(opt, -1);
    }

    ch.onchange = function() {
        var channel = document.getElementById("channelselector").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", '/channel', true);
        xhr.setRequestHeader("Content-type", "text/plain");
        xhr.send(channel);
    };
};

function refreshChannel() {
    var r = new XMLHttpRequest();
    r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        document.getElementById("channelselector").value = r.responseText;
    };
    r.open("GET", "/channel", true);
    r.send()
};

var channelRefreshTimer = setInterval(refreshChannel, 2000);

var ensembleInfoTimer = setInterval(populateEnsembleinfo, 1000);

var plotTimer = setInterval(populatePlots, 480);

function ensembleInfoTemplate() {
    var html = '<p>';
    html += ' <b>${label}</b>(${shortlabel}) EId: ${EId} SNR: ${SNR}';
    html += ' Freq corr: ${FrequencyCorrection}';
    html += ' Date:${year}-${month}-${day} ${hour}:${minutes} UTC';
    html += ' FIC CRC Errors: ${ficcrcerrors}';
    html += ' <button type=button onclick="stopPlayer()">Stop</button>';
    html += '</p>';
    html += '<table id="servicetable">'
    html += '<tr><th>Label</th> <th>SId</th> <th>bitrate</th> <th>SAd, CUs</th> <th>protection</th>';
    html += '<th>TMid, rate, channels</th>';
    html += '<th>pty</th> <th>srv/sub language</th> <th>DLS</th> <th>Frame,RS,AAC errors</th> <th>Audio Level</th> <th></th></tr>';
    html += '${services}</table>';
    return html;
}

function serviceTemplate() {
    var html = '<tr><td>${label} (${shortlabel})</td> <td>${SId}</td> <td>${bitrate}kbps</td> <td>${sad_cu}</td> <td>${protection}</td>';
    html += '<td>${samplerate}</td>';
    html += '<td>${pty}</td> <td>${language}/${subchannel_language}</td> <td></i>${dls}</i></td>';
    html += '<td>${errorcounters}</td>';
    html += '<td><canvas id="${canvasid}" width="64" height="12"></canvas></td>';
    html += '<td><button type=button onclick="setPlayerSource(${SId})">Play</button></td>';
    html += '</tr>';
    return html;
}

function tiiTemplate() {
    var html = '<li>MainId ${pattern} SubId ${comb} ${delay} samples = <b>${delay_km} km</b>';
    html += ' error: ${error}</li>';
    return html;
}

function playerLoad() {
    document.getElementById("player").load();
    document.getElementById("player").play();
}

function setPlayerSource(sid) {
    document.getElementById("player").src = "/mp3/" + sid;
    playerLoad();
}

function stopPlayer() {
    document.getElementById("player").src = "";
    playerLoad();
}

function parseTemplate(template, data) {
   return template.replace(/\$\{(\w+)\}/gi, function(match, parensMatch) {
     if (data[parensMatch] !== undefined) {
       return data[parensMatch];
     }

     return match;
   });
}

function drawCIRPeaks(cirs) {
    var canvas = document.getElementById("cirpeaks");
    var ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle="#BBFF77";
    ctx.font="8px Helvetica";

    var i = 0;
    for (cir in cirs) {
        var h = 10 + 3*(i % 3);
        ctx.fillText(i, cirs[cir].index/4, h);
        i++;
    }
}

function drawAudiolevels(services) {
    for (key in services) {
        var service = services[key];
        var id = "canvas" + service.sid;
        var canvas = document.getElementById(id);
        var ctx = canvas.getContext("2d");

        var level_db_l = -90;
        var level_db_r = -90;
        if (service.audiolevel) {
            if (service.audiolevel.left > 0) {
                level_db_l = Math.round(20 * Math.log10(service.audiolevel.left / 0x7fff));
            }
            if (service.audiolevel.right > 0) {
                level_db_r = Math.round(20 * Math.log10(service.audiolevel.right / 0x7fff));
            }

            var last_update = new Date(service.audiolevel.time * 1000);
            canvas.setAttribute("title", "Updated " + last_update);
        }

        var width_l = (level_db_l + 90) * 64/90;
        var width_r = (level_db_r + 90) * 64/90;

        var height = canvas.height / 2;

        ctx.fillStyle = "#444444";
        ctx.fillRect(0,0,width_l,height);
        ctx.fillStyle = "#DD4444";
        ctx.fillRect(0,height,width_r,2*height);
    }
};

function populateEnsembleinfo() {
    var r = new XMLHttpRequest();
    r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        var data = JSON.parse(r.responseText);

        var start_addresses = [];
        for (key in data.services) {
            var service = data.services[key];
            if (service.components) {
                var sad_ix = {};
                sad_ix["sad"] = service.components[0].subchannel.sad;
            }
            else {
                // Place them at the end
                sad_ix["sad"] = 864;
            }
            sad_ix["key"] = key;
            start_addresses.push(sad_ix);
        }

        start_addresses.sort(function(a, b) {
            return a.sad - b.sad;
        });

        var servicehtml = "";
        var imageshtml = "";
        for (ix in start_addresses) {
            var key = start_addresses[ix].key;
            var service = data.services[key];
            var s = {};
            s["label"] = service.label;
            s["shortlabel"] = service.shortlabel;
            s["SId"] = service.sid;
            if (service.components) {
                var sc = service.components[0];
                var sub = sc.subchannel;
                s["bitrate"] = sub.bitrate;
                s["sad_cu"] = sub.sad + ", " + sub.cu;
                s["protection"] = sub.protection;
                s["subchannel_language"] = sub.languagestring;

                if (sc.transportmode == "audio") {
                    s["samplerate"] = sc.ascty + ", " +
                        service.samplerate + " Hz, " +
                        service.mode + ", " +
                        service.channels;
                }
                else {
                    s["samplerate"] = sc.transportmode;
                }

            }
            else {
                s["bitrate"] = 0;
                s["sad"] = -1;
                s["protection"] = "?";
                s["samplerate"] = "";
            }

            if (service.dls) {
                var last_update = new Date(service.dls.time * 1000);
                s["dls"] = '<span title="Updated ' + last_update + '">' + service.dls.label + '</span>';
            }
            else {
                s["dls"] = "";
            }

            if (service.xpaderror.haserror) {
                var alerthtml = ' <img width=16 height=16 src="data:image/png;base64,' + png_alert + '" ';
                var tooltip = "X-PAD Length error, expected " + service.xpaderror.announcedlen +
                    " got " + service.xpaderror.len;
                alerthtml += 'title="' + tooltip + '" ';
                alerthtml += 'alt="' + tooltip + '">';
                s["dls"] += alerthtml;
            }

            s["pty"] = service.ptystring;
            s["language"] = service.languagestring;
            s["canvasid"] = "canvas" + service.sid;

            imageshtml += ' <img width=128 height=96 src="';
            if (service.mot && service.mot.data) {
                imageshtml += "data:" + service.mot.type + ";base64," +
                    service.mot.data + '">';
            }
            else {
                imageshtml += "data:image/png;base64," + png_cross + '">';
            }

            s["errorcounters"] = service.errorcounters.frameerrors + "," +
                                 service.errorcounters.rserrors + "," +
                                 service.errorcounters.aacerrors;

            servicehtml += parseTemplate(serviceTemplate(), s)
        }

        var ens = {};
        ens["label"] = data.ensemble.label;
        ens["shortlabel"] = data.ensemble.shortlabel;
        ens["EId"] = data.ensemble.id;

        ens["year"] = data.utctime.year;
        ens["month"] = data.utctime.month;
        ens["day"] = data.utctime.day;
        ens["hour"] = data.utctime.hour;
        ens["minutes"] = data.utctime.minutes;

        ens["SNR"] = data.snr;
        ens["FrequencyCorrection"] = data.frequencycorrection;
        ens["services"] = servicehtml;
        ens["ficcrcerrors"] = data.ensemble.fic.numcrcerrors;

        var ei = document.getElementById('ensembleinfo');
        ei.innerHTML = parseTemplate(ensembleInfoTemplate(), ens);

        document.getElementById('images').innerHTML = imageshtml;

        tiihtml = "<ul>";
        for (key in data.tii) {
            tiihtml += parseTemplate(tiiTemplate(), data.tii[key])
        }
        tiihtml += "</ul>";

        var tii_el = document.getElementById('tiiinfo');
        tii_el.innerHTML = tiihtml;

        drawCIRPeaks(data.cir);

        drawAudiolevels(data.services);
    };
    r.open("GET", "/mux.json", true);
    r.send()
};

function plot(data, id, scalefactor, shiftfactor, plot_ix) {
    var canvas = document.getElementById(id);
    var ctx = canvas.getContext("2d");
    if (plot_ix == 0) {
        ctx.fillStyle = "#111100";
        ctx.fillRect(0,0,data.length,150);
    }

    ctx.beginPath();
    colors = ["#FF4444", "#33FF55", "#778800"];
    if (plot_ix < colors.length) {
        ctx.strokeStyle=colors[plot_ix];
    }
    else {
        ctx.strokeStyle=colors[0];
    }
    var dataMax = 0;
    var dataMin = 1000;
    for (var i = 0; i < data.length; i++) {
        var dataScaled = 170-scalefactor*(data[i] + shiftfactor);

        if (dataMax < dataScaled) {
            dataMax = dataScaled;
        }

        if (dataMin > dataScaled) {
            dataMin = dataScaled;
        }

        if (i % 4 == 0) {
            ctx.moveTo(i/4, dataMin);
            ctx.lineTo(i/4, dataMax);

            dataMax = 0;
            dataMin = 1000;
        }
    }
    ctx.stroke();
};

function populatePlots() {
    populateSpectrum();
    populateCIR();
    populateConstellation();
};

function populateSpectrum() {
    var r = new XMLHttpRequest();
    r.onload = function(oEvent) {
        var arrayBuffer = r.response;
        if (arrayBuffer) {
            var spec = new Float32Array(arrayBuffer);
            plot(spec, "spectrum", 2, 20, 0);
        }

        r2 = new XMLHttpRequest();
        r2.onload = function(oEvent) {
            var arrayBuffer = r2.response;
            if (arrayBuffer) {
                var spec = new Float32Array(arrayBuffer);
                plot(spec, "spectrum", 2, 20, 1);
            }
        };
        r2.open("GET", "/nullspectrum", true);
        r2.responseType = "arraybuffer";
        r2.send(null);
    };
    r.open("GET", "/spectrum", true);
    r.responseType = "arraybuffer";
    r.send(null);
};

function populateCIR() {
    var r = new XMLHttpRequest();
    r.onload = function(oEvent) {
        var arrayBuffer = r.response;
        if (arrayBuffer) {
            var cir = new Float32Array(arrayBuffer);
            plot(cir, "cir", 4, 30, 0)
        }
    };
    r.open("GET", "/impulseresponse", true);
    r.responseType = "arraybuffer";
    r.send(null);
}

function populateConstellation() {
    var r = new XMLHttpRequest();
    r.onload = function(oEvent) {
        var arrayBuffer = r.response;
        if (arrayBuffer) {
            var data = new Float32Array(arrayBuffer);

            var squeeze = 4;

            var canvas = document.getElementById("constellation");
            var ctx = canvas.getContext("2d");
            ctx.fillStyle = "#111100";
            ctx.fillRect(0,0,data.length / squeeze,180);

            ctx.beginPath();
            ctx.strokeStyle="rgba(255, 100, 0, 0.8)";
            for (var i = 0; i < data.length; i++) {
                var x = i / squeeze;
                var y = (data[i] + 180) / 2;
                // Draw a little cross
                ctx.moveTo(x-1, y);
                ctx.lineTo(x+1, y);
                ctx.moveTo(x, y-1);
                ctx.lineTo(x, y+1);
            }
            ctx.stroke();
        }
    };
    r.open("GET", "/constellation", true);
    r.responseType = "arraybuffer";
    r.send(null);
}


