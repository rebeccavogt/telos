function fetchPrice(a) {
    var e = $("#keysToBuy").val();
    "SELL" == a && (e = $("#keysToSell").val());
    var t = "https://bancor3d.com/api/get_price?amount=" + e;
    $.get(t, function (e) {
        if ("BUY" == a && e.price_buy) {
            var t = e.price_buy;
            $("#current-money-buy").html("@ " + t.toFixed(4) + " EOS")
        } else if ("SELL" == a && e.price_sell) {
            t = e.price_sell;
            $("#current-money-sell").html("@ " + t.toFixed(4) + " EOS")
        }
    })
}
Date.prototype.Format = function (e) {
    var t = {
        "M+": this.getMonth() + 1,
        "d+": this.getDate(),
        "h+": this.getHours(),
        "m+": this.getMinutes(),
        "s+": this.getSeconds(),
        "q+": Math.floor((this.getMonth() + 3) / 3),
        S: this.getMilliseconds()
    };
    for (var a in /(y+)/.test(e) && (e = e.replace(RegExp.$1, (this.getFullYear() + "").substr(4 - RegExp.$1.length))), t) 
        new RegExp("(" + a + ")").test(e) && (e = e.replace(RegExp.$1, 1 == RegExp.$1.length ? t[a] : ("00" + t[a]).substr(("" + t[a]).length)));
    return e
}, $(function () {
    tpAccount = null, 
    language = "en", 
    languageMapDefault = {
        keynes: "凯恩斯",
        modal_proud: "，我为你自豪。你是第 ",
        modal_volunteer: " 实验志愿者。",
        login: "请在安装钱包，并且登录后参与实验。",
        not_start: "凯恩斯实验尚未开始。",
        ended: "凯恩斯实验已经结束。你持有的Keys将会用于哈耶克实验，你的EOS余额仍然可以提出。",
        success: "成功！",
        select: "请选择EOS账户",
        withdraw_small: "凯恩斯没有0.1 EOS 以下的零钱。可以通过邀请更多好友获得更多分红。",
        buy_small: "凯恩斯实验最小金额为0.1 EOS."
    }, 
    languageMap = languageMapDefault, 
    scatter = null, 
    bitportal = null, 
    money = null, 
    isStarted = !1, 
    isEnded = !1, 
    endTime = 1538121600, // Friday, September 28, 2018 8:00:00 AM GMT
    endTime = 1538222400, // Saturday, September 29, 2018 12:00:00 PM GMT
    base = 1e6, 
    index = 0, 
    advices = ["The long run is a misleading guide to current affairs. In the long run we are all dead.", "Indeed the world is ruled by little else.", "Ideas shape the course of history.", "The difficulty lies not so much in developing new ideas as in escaping from old ones.", "The market can stay irrational longer than you can stay solvent.", "Words ought to be a little wild, for they are the assault of thoughts on the unthinking.", "Education: the inculcation of the incomprehensible into the indifferent by the incompetent.", "Ideas shape the course of history.", "The avoidance of taxes is the only intellectual pursuit that still carries any reward.", "Nothing mattered except states of mind, chiefly our own."];
    
    var contractName = "bancor3dcode",
        nodes = ["api1.eosasia.one", "eos.greymass.com"],
        rndNodeIdx = Math.floor(Math.random() * nodes.length);
    network = {
        protocol: "https",
        host: nodes[rndNodeIdx],
        port: 443,
        blockchain: "eos",
        chainId: "aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906",
        host_be: "bancor3d.com",
        port_be: "80"
    };
    var a, n, o = network.protocol,
        l = network.host,
        r = network.port,
        s = network.chainId,
        g = network.host_be,
        d = network.port_be;
    if (api = Eos({
            httpEndpoint: o + "://" + l + ":" + r,
            chainId: s
        }), console.log(o + "://" + l + ":" + r), document.addEventListener("scatterLoaded", function (e) {
            scatter = window.scatter, eos = scatter.eos(network, Eos, {}, "https")
        }), setTimeout(function () {
            localStorage.getItem("account") || scatter || Dialog.init(languageMap.login)
        }, 3e3), localStorage.getItem("account")) {
        var c = getLocalAccountName(),
            u = " Logout",
            h = "Login";
        language && "zh" != language || (u = " 注销", h = "登录"), 
            $("#log-btn").html(c + u), 
            $("#refUrl").html("https://bancor3d.com?ref=" + c), 
            getEOSBalance(), 
            updateGameStats()
    } else $("#log-btn").html(h), $(".container-main").show();

    function updateKeysToBuyQty(e) {
        if (isStarted)
            if (isEnded) Dialog.init(languageMap.ended);
            else {
                var t = $("#keysToBuy").val(),
                    a = (t = parseInt(t)) + e;
                $("#keysToBuy").val(a), fetchPrice("BUY")
            }
        else Dialog.init(languageMap.not_start)
    }

    function initAccount() {
        return localStorage.getItem("account") || scatter ? isStarted ? !isEnded || (Dialog.init(languageMap.ended), !1) : (Dialog.init(languageMap.not_start), !1) : (Dialog.init(languageMap.login), !1)
    }

    function getEOSBalance() {
        var e = getLocalAccountName();
        api.getCurrencyBalance("eosio.token", e, "EOS").then(function (e) {
            var t = 0;
            e && 0 < e.length && (t = e[0]);
            var a = "Balance ";
            language && "zh" != language || (a = "账户金额 "), $("#eos-balance").html(a + t)
        })
    }

    function updateGameStats() {
        var e = getLocalAccountName(),
            t = "";
        "80" != d && (t = ":" + d);
        var s, c, u, a = o + "://" + g + t + "/api/get_game?owner=" + e;
        u = language && "zh" != language ? (s = "Total ", c = " Days ", " Hours") : (s = "总计 ", c = " 天 ", " 小时 "), $.ajax({
            url: a,
            success: function (e) {
                var t = e.user || {
                        e: 0,
                        p: 0,
                        k: 0,
                        index: 0
                    },
                    a = e.game || {};
                endTime = a.t, 
                isStarted = 0 < endTime, 
                isEnded = isStarted && (new Date).getTime() > 1e3 * endTime, 
                index = t.index, 
                0 == index && (index = a.n, 0 == index && (index = 1)), 
                index += 23, 
                isStarted || ($("#headtimer").html(languageMap.not_start), 
                $("#boxtimer").html(languageMap.not_start), 
                endTime = 1538121600, 
                endTime = 1538222400), 
                isEnded && ($("#headtimer").html(languageMap.ended), 
                    $("#headtimer3").html(languageMap.ended), 
                    $("#boxtimer").html(languageMap.ended));
                var n = a.p / 1e4;
                $(".pot").html(n.toFixed(2) + " EOS"), 
                $("#rewards").html(t.p / 1e4 + " EOS"), 
                $(".gains").html(t.p / 1e4 + " EOS"), 
                $("#your-keys").html(t.k / 1e4 + " KEY"), 
                $("#total-keys").html(s + a.k / 1e4 + " Keys");

                var o = (new Date).getTime() / 1e3,
                    i = "";
                if (0 == t.t) i = "0" + u;
                else {
                    var l = Math.floor((o - t.t) / 3600) + 1 || 0,
                        r = Math.floor(l / 24);
                    l -= 24 * r, 0 < r && (i = r.toString() + c), i = i + l.toString() + u
                }
                $("#hodl-time").html(i)
            },
            error: function () {
                $(".pot").html("0 EOS"), 
                $("#rewards").html("0 EOS"), 
                $(".gains").html("0 EOS"), 
                $("#your-keys").html("0 KEY"), 
                $("#total-keys").html(s + "0 Keys"), 
                $("#hodl-time").html("0" + c), 
                $("#current-money-sell").html("@ 0 EOS"), 
                $("#current-money-buy").html("@ 0 EOS")
            }
        }), setTimeout(updateGameStats, 6e4)
    }

    function getLocalAccountName() {
        var e = null;
        return localStorage.getItem("account") && (e = JSON.parse(localStorage.getItem("account")).name), e
    }

    function setIdentity(e) {
        var identity = e.accounts[0];
        console.log(identity), localStorage.setItem("account", JSON.stringify(identity));
        var a = identity.name,
            n = " Logout";
        return language && "zh" != language || (n = " 注销"), 
            $("#log-btn").html(a + n), 
            $("#refUrl").html("https://bancor3d.com?ref=" + identity.name), 
            $(".container-main").hide(), 
            getEOSBalance(), 
            updateGameStats(), 
            identity
    }

    function S(e) {
        function t(e) {
            return e < 10 && (e = "0" + e), e
        }
        var a = parseInt(e / 86400),
            n = parseInt(e % 86400 / 60 / 60),
            o = parseInt(e / 60 % 60),
            i = parseInt(e % 60),
            l = (n = t(n)) + ":" + (o = t(o)) + ":" + (i = t(i)),
            r = " Days ";
        return language && "zh" !== language || (r = " 天 "), 0 < a && (l = "" + a + r + l), l
    }
    ref = (a = new RegExp("(^|&)" + "ref" + "=([^&]*)(&|$)"), null != (n = window.location.search.substr(1).match(a)) ? unescape(n[2]) : null), ref && ref.length <= 12 ? ref = ref : ref = "", 
        $("#btnAdd10").click(function () {
            updateKeysToBuyQty(10)
        }), $("#btnAdd100").click(function () {
            updateKeysToBuyQty(100)
        }), $("#btnAdd200").click(function () {
            updateKeysToBuyQty(200)
        }), $("#btnAdd500").click(function () {
            updateKeysToBuyQty(500)
        }), $("#btnAdd1000").click(function () {
            updateKeysToBuyQty(1e3)
        }), $("#tixBuy").click(function () {
            if (initAccount()) {
                var e = $("#current-money-buy").html();
                if (!e || e.split(" ").length < 3) return Dialog.init("Please input number!"), !1;
                var t = e.split(" "),
                    n = parseFloat(t[1]);
                if (n < .1) Dialog.init(languageMap.buy_small);
                else {
                    if (minKeys = 1, $("#keysToBuy").val() < minKeys) return Dialog.init("at least " + minKeys + " keys!"), void $("#keysToBuy").val(minKeys);
                    var o = parseInt($("#keysToBuy").val()).toString() + "-" + ref;
                    scatter.getIdentity({
                        accounts: [network]
                    }).then(function (e) {
                        var t = e.accounts[0],
                            a = {
                                authorization: t.name + "@" + t.authority,
                                broadcast: !0,
                                sign: !0
                            };
                        eos.transfer(t.name, contractName, n.toFixed(4) + " EOS", o, a).then(function (e) {
                            getEOSBalance(), updateGameStats();
                            var t = getLocalAccountName();
                            t = t.charAt(0).toUpperCase() + t.substr(1);
                            var a = index;
                            a += a % 10 == 1 ? "st" : a % 10 == 2 ? "nd" : a % 10 == 3 ? "rd" : "th";
                            var n = Math.floor(1e3 * Math.random() % advices.length),
                                o = advices[n],
                                i = '<p style="width: 280px;"><b>' + t + "</b>" + languageMap.modal_proud + "<b>" + a + "</b>" + languageMap.modal_volunteer + "</p>",
                                l = '<p style="width: 280px;">' + o + "</p> ",
                                r = "<b>" + languageMap.keynes + "</b>";
                            $("#modal-text").html(i), $("#modal-advice").html(l), $("#modal-keynes").html(r), $("#modal-date").html((new Date).toLocaleDateString()), $("#exampleModal").modal()
                        }).catch(function (e) {
                            e = JSON.parse(e), Dialog.init("Ooops... " + e.error.details[0].message.replace("assertion failure with message: ", " "))
                        }), setIdentity(e)
                    })
                }
            }
        }), $("#tixSell").click(function () {
            if (initAccount()) {
                var e = parseFloat($("#keysToSell").val());
                if (minKeys = 1, e < minKeys) return Dialog.init("at least " + minKeys + " keys!"), void $("#keysToSell").val(minKeys);
                var n = e.toFixed(4) + " KEY";
                scatter.getIdentity({
                    accounts: [network]
                }).then(function (e) {
                    var t = setIdentity(e),
                        a = {
                            authorization: t.name + "@" + t.authority,
                            broadcast: !0,
                            sign: !0
                        };
                    eos.contract(contractName, a).then(function (e) {
                        e.sell(t.name, n, a).then(function (e) {
                            updateGameStats(), Dialog.init(languageMap.success)
                        }).catch(function (e) {
                            e = JSON.parse(e), Dialog.init("Ooops... " + e.error.details[0].message.replace("assertion failure with message: ", " "))
                        })
                    })
                })
            }
        }), $("#log-btn-main").click(function () {
            localStorage.getItem("account") || scatter ? (localStorage.removeItem("account"), scatter.getIdentity({
                accounts: [network]
            }).then(function (e) {
                setIdentity(e)
            }).catch(function (e) {
                console.log(e)
            })) : Dialog.init(languageMap.login)
        }), $("#log-btn").click(function () {
            localStorage.getItem("account") || scatter ? $(this).html().endsWith("Logout") || $(this).html().endsWith("注销") ? scatter.forgetIdentity().then(function () {
                ! function () {
                    var e = "Balance ",
                        t = "Login";
                    language && "zh" != language || (e = "账户金额 ", t = "登录");
                    localStorage.removeItem("account"), $("#log-btn").html(t), $("#your-keys").html("0.00"), $("#eos-balance").html(e + "0.00 EOS"), $("#scammed").html("0.0000 EOS"), $("#advice").html("0.0000 EOS"), $(".gains").html("0.0000 EOS"), $(".gains-usdt").html("≙ 0.00 USDT"), $("#hodl-time").html("0 Hours"), $("#refUrl").html("https://bancor3d.com?ref=dinnerwinner"), $(".scatter").show()
                }()
            }) : (localStorage.removeItem("account"), scatter.getIdentity({
                accounts: [network]
            }).then(function (e) {
                setIdentity(e)
            }).catch(function (e) {
                console.log(e)
            })) : Dialog.init(languageMap.login)
        }), $("#language").click(function () {
            var e = localStorage.getItem("language");
            languageMap = e && "en" == e ? ($("#text-rules").attr("href", "./Bancor3DRules_v1.0_zh.pdf"), language = "zh", $("#language").text("English"), $("#purchaseTab").text("购买"), $("#sellTab").text("出售"), $("#inviteTab").text("邀请奖励"), $("#potTab").text("凯恩斯实验"), $("#text-reward").text("凯恩斯大奖"), $("#text-keys").text("Key金额"), $("#text-hodl").text("持Key时间"), $("#text-eos-reward").text("EOS分红"), $("#text-rules").text("规则"), $("#log-btn").text("登录"), $("#log-btn-main").text("登录"), $("#tixBuy").text("购买"), $("#text-purchaseTab").text("购买"), $("#tixSell").text("出售 KEY"), $("#text-sell").text("出售"), $("#text-withdraw").text("提出"), $("#withdrawEarnings").text("提出"), $("#text-refer").text("邀请好友，获得10%奖励:"), $("#exampleModalLabel").text("祝贺你"), $("#text-close").text("关闭"), $("#text-bancor3d").text("Bancor3D.com  一场加速版经济学实验"), languageMapDefault) : ($("#text-rules").attr("href", "./Bancor3DRules_v1.0.pdf"), language = "en", $("#language").text("中文"), $("#purchaseTab").text("Purchase"), $("#sellTab").text("Sell"), $("#inviteTab").text("Invite Url"), $("#potTab").text("Keynes Experiment"), $("#text-reward").text("Keynes' Pot"), $("#text-keys").text("Your Keys"), $("#text-hodl").text("HODL"), $("#text-eos-reward").text("Rewards"), $("#text-rules").text("Rules"), $("#log-btn").text("Login"), $("#log-btn-main").text("Login"), $("#tixBuy").text("Enroll"), $("#text-purchaseTab").text("Purchase"), $("#tixSell").text("Sell KEY"), $("#text-sell").text("Sell"), $("#text-withdraw").text("Withdraw"), $("#withdrawEarnings").text("Withdraw"), $("#text-refer").text("Refer to get 10%:"), $("#exampleModalLabel").text("Congratulations"), $("#text-close").text("Close"), $("#text-bancor3d").text("Bancor3D.com  An Accelerated Economic Game"), {
                keynes: "J. M. Keynes",
                modal_proud: ", I am so proud of you. You are the ",
                modal_volunteer: " volunteer for the experiment.",
                login: "Please install Scatter and login for the experiment.",
                not_start: "The Keynes' experiment has not been started!",
                ended: "The Keynes' experiment is ended. You can still claim your EOS balance, and your keys will be enrolled in Hayek' experiment.",
                success: "Success!",
                select: "Please select an EOS Account",
                withdraw_small: "Keynes does not like small change less than 0.1 EOS. Try invite more friends to get more bonus.",
                buy_small: "Minimum ticket to Keynes' Experiment is 0.1 EOS."
            }), localStorage.setItem("language", language), getEOSBalance()
        }), $("#withdrawEarnings").click(function () {
            if (localStorage.getItem("account") || scatter)
                if (isStarted) {
                    var e = $("#rewards").html().split(" ");
                    if (e.length < 1 && (e = ["0"]), parseFloat(e[0]) <= .1) Dialog.init(languageMap.withdraw_small);
                    else scatter.getIdentity({
                        accounts: [network]
                    }).then(function (e) {
                        var t = setIdentity(e),
                            a = {
                                authorization: t.name + "@" + t.authority,
                                broadcast: !0,
                                sign: !0
                            };
                        eos.contract(contractName, a).then(function (e) {
                            e.withdraw(t.name, a).then(function (e) {
                                getEOSBalance(), updateGameStats(), Dialog.init(languageMap.success)
                            }).catch(function (e) {
                                e = JSON.parse(e), Dialog.init("Ooops... " + e.error.details[0].message.replace("assertion failure with message: ", " "))
                            })
                        })
                    })
                } else Dialog.init(languageMap.not_start);
            else Dialog.init(languageMap.login)
        }), setTimeout(updateGameStats, 1e3), fetchPrice("BUY"), fetchPrice("SELL"),
        function e() {
            if (!isEnded)
                if (isStarted) {
                    if ($("#startedContainer").show(), $("#toStartContainer").hide(), endTime) {
                        if ((t = parseInt(endTime - (new Date).getTime() / 1e3)) <= 0 && !isEnded) return $("#headtimer").html("Your network may have problems!"), void $("#boxtimer").html("Your network may have problems!");
                        a = S(t), $("#headtimer").html(a), $("#boxtimer").html(a)
                    }
                    setTimeout(e, 1e3)
                } else {
                    var t;
                    $("#startedContainer").hide(), $("#toStartContainer").show(), (t = parseInt(endTime - (new Date).getTime() / 1e3)) < 0 && (t = 0);
                    var a = S(t);
                    setTimeout(e, 1e3)
                }
        }()
});