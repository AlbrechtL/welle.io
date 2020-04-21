var channels = [ "5A", "5B", "5C", "5D", "6A", "6B", "6C", "6D", "7A", "7B",
"7C", "7D", "8A", "8B", "8C", "8D", "9A", "9B", "9C", "9D", "10A", "10B",
"10C", "10D", "11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D", "13A",
"13B", "13C", "13D", "13E", "13F"];

var fft_window_placements = [ "StrongestPeak", "EarliestPeakWithBinning", "ThresholdBeforePeak" ];

var png_noslide = "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";

var png_alert = "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAk1BMVEWgAAD/AAD/////+/v/+Pj/2tr/T0//1NT/5eX/mJj/pKT/8/P/7Oz/y8v/4eH/Dg7/19f/R0f/vb3/Xl7/eHj/Njb/6+v/sbH/fHz/iYn/QUH/cXH/nZ3/HBz/JSX/bGz/jo7/xMT/KSn/ra3/Zmb/PT3/sLD/hIT/YGD/hob/VVX/Fhb/t7f/yMj/TU3/OTn/oqILUJpfAAAAAXRSTlMAQObYZgAAAFVJREFUOMuV0EEOACEIA0Dn/5/em8kqWumxTAJhjGZI8yAEIQDugiASIIgE5uAg/IE2sAJNYAdaQAU0gBqonlwCxzyCtdlEAlKxbnUH+5lCMnh51DUfEC4CA1JxhXIAAAAASUVORK5CYII="

var png_chevron_right = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAOxAAADsQBlSsOGwAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAABpSURBVEiJ7c4xDoJAEEbhL8rtPICVHQ3XoaLQ2sSzcA6OQEgIrI3FVmaNbmPm1f+8NwTBX3HGgEMt+YqEa41IHki4ReQdfRZIaH8pP2HJ5A80IYcO+0t+x7H0sHQ4YsKMC7YPHwyCL3gCWFcmgwfCKVwAAAAASUVORK5CYII="

var png_chevron_down = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAOxAAADsQBlSsOGwAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAB0SURBVEiJ7Y87CoAwEAVHD2khqIUgCF5HUPBzUL9NgiGoSKKN7MAW4cEMAUEQnlAAycUWA6WPPAcWdZm1RcAErEDlGmiATd3M8ZNYvfXWuQYCoLUijSXvgdA1oCO1ITRv8JXfRV6Tn0XGt+VmJP1KLgh/ZQd6AiOdJgUioQAAAABJRU5ErkJggg=="

window.onload = function() {
    var chevron_right = '<img width=24 height=24 src="data:image/png;base64,' +
        png_chevron_right + '" />';
    var chevron_down = '<img width=24 height=24 src="data:image/png;base64,' +
        png_chevron_down + '" />';

    var spectrum_block = document.getElementById('block_spectrum');
    spectrum_block.getElementsByClassName('title')[0]
        .getElementsByClassName('chevron')[0]
        .innerHTML = chevron_right;

    var cir_block = document.getElementById('block_cir');
    cir_block.getElementsByClassName('title')[0]
        .getElementsByClassName('chevron')[0]
        .innerHTML = chevron_right;

    var constellation_block = document.getElementById('block_constellation');
    constellation_block.getElementsByClassName('title')[0]
        .getElementsByClassName('chevron')[0]
        .innerHTML = chevron_right;

    var tii_block = document.getElementById('block_tii');
    tii_block.getElementsByClassName('title')[0]
        .getElementsByClassName('chevron')[0]
        .innerHTML = chevron_right;

    toggle_func = function(block) {
        var fold = block.getElementsByClassName('title')[0]
            .getElementsByClassName('chevron')[0];
        var style = block.getElementsByClassName('data')[0].style;
        if (style.display != "none") {
            style.display = 'none';
            fold.innerHTML = chevron_right;
            return false;
        }
        else {
            style.display = 'block';
            fold.innerHTML = chevron_down;
            return true;
        }
    };

    spectrum_block.onclick = function() {
        if (toggle_func(spectrum_block)) {
            populateSpectrumPlots(480);
        }
        else {
            clearInterval(plotSpectrumTimer);
        }
    };

    cir_block.onclick = function() {
        if (toggle_func(cir_block)) {
            populateCIRPlots(480);
        }
        else {
            clearInterval(plotCIRTimer);
        }
    };

    constellation_block.onclick = function() {
        if (toggle_func(constellation_block)) {
            populateConstellationPlots(480);
        }
        else {
            clearInterval(plotConstellationTimer);
        }
    };

    tii_block.onclick = function() { toggle_func(tii_block); };

    var ch = document.getElementById("channelselector");

    for (i in channels) {
        var opt = document.createElement("option");
        opt.text = channels[i];
        ch.options.add(opt, -1);
    }

    ch.onchange = function() {
        var channel = document.getElementById("channelselector").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", '/channel', true);
        xhr.setRequestHeader("Content-type", "text/plain");
        xhr.send(channel);
    };

    var fftw = document.getElementById("fftwindowselector");

    for (i in fft_window_placements) {
        var opt = document.createElement("option");
        opt.text = fft_window_placements[i];
        fftw.options.add(opt, -1);
    }

    fftw.onchange = function() {
        var fft_window = document.getElementById("fftwindowselector").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", '/fftwindowplacement', true);
        xhr.setRequestHeader("Content-type", "text/plain");
        xhr.send(fft_window);
    };

    document.getElementById("coarsecheckbox").onclick = function() {
        var fft_window = document.getElementById("fftwindowselector").value;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", '/enablecoarsecorrector', true);
        xhr.setRequestHeader("Content-type", "text/plain");
        if (document.getElementById("coarsecheckbox").checked) {
            xhr.send(1);
        }
        else {
            xhr.send(0);
        }
    }
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

function ensembleInfoTemplate() {
    var html = '';
    html += ' <h1><abbr title="Ensemble long and short labels defined in FIG1">${label} (${shortlabel})</abbr></h1>';
    html += ' <h2><abbr title="Ensemble long and short labels defined in FIG2">${fig2label}</abbr></h2>';
    html += ' <div align="right"><p><abbr title="${hw_name}, ${sw_name}">This is welle-cli build version ${version}</abbr></p></div>';
    html += ' <table id="servicetable">';
    html += ' <tr><th>Ensemble ID </th>';
    html += ' <th>ECC </th>';
    html += ' <th>SNR </th>';
    html += ' <th>RX gain </th>';
    html += ' <th>Freq corr</th>';
    html += ' <th>Date</th></th>';
    html += ' <th><abbr title="Local Time Offset">LTO</abbr></th></th>';
    html += ' <th>FIC CRC Errors</th>';
    html += ' <th>Tuned at</th>';
    html += ' </tr>';
    html += ' <tr><td>${EId}</td>';
    html += ' <td>${ecc}</td>';
    html += ' <td>${SNR}</td>';
    html += ' <td>${gain}</td>';
    html += ' <td>${FrequencyCorrection}</td>';
    html += ' <td>${year}-${month}-${day} ${hour}:${minutes} UTC</td>';
    html += ' <td>${lto}</td>';
    html += ' <td>${ficcrcerrors}</td>';
    html += ' <td>${lastchannelchange}</td></tr>';
    html += ' </table><br>    <button type=button onclick="stopPlayer()">Stop</button><br><br>';
    html += '';
    html += '<table id="servicetable">';
    html += '<tr><th>FIG1 Label (Short label)<br>FIG2 Label</th> ';
    html += '<th><abbr title="Service ID">SId</abbr></th> ';
    html += '<th>Bitrate</th> <th><abbr title="Start CU Address, used CUs">CU info</abbr></th> ';
    html += '<th><abbr title="Protection level">Prot.Lev.</abbr></th>';
    html += '<th><abbr title="Transmission Mode, rate, channels">Technical details</abbr></th>';
    html += '<th><abbr title="Programme type">PTy</abbr></th>';
    html += '<th><abbr title="Service language, Subchannel language">Languages</abbr></th> <th id="dls">DLS</th>';
    html += '<th><abbr title="Frame, Reed Solomon, AAC errors">Errors</abbr></th>';
    html += '<th><abbr title="red: right, black: left">Audio Level</abbr></th> <th></th></tr>';
    html += '${services}</table>';
    return html;
}

function serviceTemplate() {
    var html = '<tr><td>${label} (${shortlabel})<br>${fig2label}</td> <td>${SId}</td> <td>${bitrate}&nbsp;kbps</td> <td>${sad_cu}</td> <td>${protection}</td>';
    html += '<td>${techdetails}</td>';
    html += '<td>${pty}</td> <td>${language}<br>${subchannel_language}</td> <td></i>${dls}</i></td>';
    html += '<td>${errorcounters}</td>';
    html += '<td><canvas id="${canvasid}" width="64" height="12"></canvas></td>';
    html += '<td><button type=button ${buttondisabled} class="${buttonclass}" onclick="setPlayerSource(${SId})">Play</button></td>';
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

var slide_modal = document.getElementById('mySlideModal');
var slideimg = document.getElementById("slideimg");
var slidecaption = document.getElementById("slidecaption");

function showSlide(sid, last_update_time) {
    slideimg.src = "slide/" + sid + "?cachebreak=" + last_update_time;
    var last_update = new Date(last_update_time * 1000);
    slidecaption.innerHTML = last_update;
    slide_modal.style.display = "block";
}

var slideclose = document.getElementsByClassName("slideclose")[0];
slideclose.onclick = function() {
    slide_modal.style.display = "none";
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
            var sad_ix = {};
            if (service.components) {
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
        for (ix in start_addresses) {
            var key = start_addresses[ix].key;
            var service = data.services[key];
            var s = {};
            s["label"] = service.label.label;
            s["fig2label"] = service.label.fig2label;
            s["shortlabel"] = service.label.shortlabel;
            s["SId"] = service.sid;
            s["buttondisabled"] = "disabled";
            s["buttonclass"] = "disabled";
            if (service.components) {
                var sc = service.components[0];
                var sub = sc.subchannel;
                s["bitrate"] = sub.bitrate;
                s["sad_cu"] = sub.sad + ", " + sub.cu;
                s["protection"] = sub.protection;
                s["subchannel_language"] = sub.languagestring;

                if (sc.transportmode == "audio") {
                    s["techdetails"] = sc.ascty + ", " +
                        service.samplerate + " Hz, " +
                        service.mode + ", " +
                        service.channels;
                    s["buttondisabled"] = "";
                    s["buttonclass"] = "";
                }
                else {
                    s["techdetails"] = sc.transportmode + ", DSCTy=" + sc.dscty;
                }
            }
            else {
                s["bitrate"] = 0;
                s["sad"] = -1;
                s["protection"] = "?";
                s["techdetails"] = "";
            }

            s["dls"] = "";

            if (service.mot && service.mot.time > 0) {
                s["dls"] += '<button type=button onclick="showSlide(';
                s["dls"] += service.sid + ', ' + service.mot.time;
                s["dls"] += ')">SLS</button>';
            }

            if (service.dls) {
                var last_update = new Date(service.dls.time * 1000);
                s["dls"] += ' <span title="Updated ' + last_update + '">' + service.dls.label + '</span>';
            }

            if (service.xpaderror && service.xpaderror.haserror) {
                var alerthtml = ' <img width=16 height=16 src="data:image/png;base64,' + png_alert + '" ';
                var tooltip = "X-PAD Length error, expected " + service.xpaderror.announcedlen +
                    " got " + service.xpaderror.len;
                alerthtml += 'title="' + tooltip + '" ';
                alerthtml += 'alt="' + tooltip + '">';
                s["dls"] += alerthtml;
            }
            s["dls"] += "</td>";

            s["pty"] = service.ptystring;
            s["language"] = service.languagestring;
            s["canvasid"] = "canvas" + service.sid;

            if (service.errorcounters) {
                s["errorcounters"] = service.errorcounters.frameerrors + "," +
                                     service.errorcounters.rserrors + "," +
                                     service.errorcounters.aacerrors;
            }
            else {
                s["errorcounters"] = "";
            }

            servicehtml += parseTemplate(serviceTemplate(), s)
        }

        var ens = {};
        ens["label"] = data.ensemble.label.label;
        ens["fig2label"] = data.ensemble.label.fig2label;
        ens["shortlabel"] = data.ensemble.label.shortlabel;
        ens["EId"] = data.ensemble.id;
        ens["ecc"] = data.ensemble.ecc;

        ens["year"] = data.utctime.year;
        ens["month"] = data.utctime.month;
        ens["day"] = data.utctime.day;
        ens["hour"] = data.utctime.hour;
        ens["minutes"] = data.utctime.minutes;
        ens["lto"] = data.utctime.lto;

        ens["gain"] = data.receiver.hardware.gain.toFixed(1);
        document.getElementById("fftwindowselector").value = data.receiver.software.fftwindowplacement;
        document.getElementById("coarsecheckbox").checked = data.receiver.software.coarsecorrectorenabled;

        ens["version"] = data.receiver.software.version;
        ens["hw_name"] = data.receiver.hardware.name;
        ens["sw_name"] = data.receiver.software.name;
        ens["SNR"] = data.demodulator.snr.toFixed(1);
        ens["FrequencyCorrection"] = data.demodulator.frequencycorrection;
        ens["services"] = servicehtml;
        ens["ficcrcerrors"] = data.demodulator.fic.numcrcerrors;
        var lcc = new Date(data.receiver.software.lastchannelchange * 1000);
        ens["lastchannelchange"] = lcc.toISOString();

        var ei = document.getElementById('ensembleinfo');
        ei.innerHTML = parseTemplate(ensembleInfoTemplate(), ens);

        tiihtml = "<ul>";
        for (key in data.tii) {
            tiihtml += parseTemplate(tiiTemplate(), data.tii[key])
        }
        tiihtml += "</ul>";

        var tii_el = document.getElementById('tiiinfo');
        tii_el.innerHTML = tiihtml;

        drawCIRPeaks(data.cir_peaks);

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

function populateSpectrumPlots(interval) {
    populateSpectrum();
    if (interval > 0) {
        plotSpectrumTimer = setTimeout(function() { populateSpectrumPlots( interval); }, interval);
    }
};

function populateCIRPlots(interval) {
    populateCIR();
    if  (interval > 0) {
        plotCIRTimer = setTimeout(function() { populateCIRPlots( interval); }, interval);
    }
};

function populateConstellationPlots(interval) {
    populateConstellation();
    if  (interval > 0) {
        plotConstellationTimer = setTimeout(function() { populateConstellationPlots( interval); }, interval);
    }
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


