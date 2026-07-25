param(
    [string]$ClientHeaderPath = "SDK/af_sdk/sdk_client/include/NetTVSDKClientInterface.h",
    [string]$CommonHeaderPath = "SDK/af_sdk/sdk_share/include/NetTVSDKCommon.h",
    [string]$DemoDir = "SDK/af_sdk/sdk_client/demo",
    [string]$OutputDir = "docs/helpndoc_tvsdk_client_topics",
    [string]$RootTopicTitle = "SDK client接口定义",
    [string]$InterfaceTopicTitle = "接口定义",
    [string]$StructTopicTitle = "接口参数结构体",
    [string]$DemoTopicTitle = "接口调用Demo"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:StructTopicNameSet = @{}
$script:InterfaceTopicNameSet = @{}
$script:DemoTopicFileSet = @{}

function ConvertTo-HtmlText([string]$Text) {
    if ($null -eq $Text) {
        return ""
    }
    $value = $Text.Trim()
    $value = $value -replace '\\\|', '|'
    $value = $value -replace '`', ''
    $value = $value -replace '\*\*', ''
    return [System.Net.WebUtility]::HtmlEncode($value)
}

function ConvertTo-HtmlCodeText([string]$Text) {
    if ($null -eq $Text) {
        return ""
    }
    return [System.Net.WebUtility]::HtmlEncode($Text)
}

function CleanInline([string]$Text) {
    if ($null -eq $Text) {
        return ""
    }
    $value = $Text.Trim()
    $value = $value -replace '\\\|', '|'
    $value = $value -replace '`', ''
    $value = $value -replace '\*\*', ''
    $value = $value -replace '\s+', ' '
    return $value.Trim()
}

function ConvertTo-HelpId([string]$Title) {
    $knownIds = @{
        "SDK client接口定义" = "SDK_Client_Interface_Definition"
        "接口定义" = "SDK_Client_Interface_List"
        "接口参数结构体" = "SDK_Client_Struct_List"
        "接口调用Demo" = "SDK_Client_Demo_List"
    }
    if ($knownIds.ContainsKey($Title)) {
        return $knownIds[$Title]
    }

    $id = ($Title -replace '[^A-Za-z0-9_]', '_').Trim('_')
    if ([string]::IsNullOrWhiteSpace($id)) {
        $id = "topic"
    }
    if ($id.Length -gt 60) {
        $id = $id.Substring(0, 60)
    }
    return $id
}

function ConvertTo-SafeFileName([string]$Title) {
    $name = ($Title -replace '[\\/:*?"<>|]', '_').Trim()
    if ($name.Length -gt 120) {
        $name = $name.Substring(0, 120)
    }
    return $name
}

function Escape-Pascal([string]$Text) {
    return $Text -replace "'", "''"
}

function Split-ParameterList([string]$ParamText) {
    $params = New-Object System.Collections.Generic.List[string]
    if ([string]::IsNullOrWhiteSpace($ParamText) -or $ParamText.Trim() -eq "void") {
        return $params.ToArray()
    }

    $sb = New-Object System.Text.StringBuilder
    $parenDepth = 0
    foreach ($ch in $ParamText.ToCharArray()) {
        if ($ch -eq '(') {
            $parenDepth++
        } elseif ($ch -eq ')' -and $parenDepth -gt 0) {
            $parenDepth--
        }

        if ($ch -eq ',' -and $parenDepth -eq 0) {
            $value = $sb.ToString().Trim()
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $params.Add($value)
            }
            [void]$sb.Clear()
            continue
        }

        [void]$sb.Append($ch)
    }

    $last = $sb.ToString().Trim()
    if (-not [string]::IsNullOrWhiteSpace($last)) {
        $params.Add($last)
    }
    return $params.ToArray()
}

function Get-ParamName([string]$ParamDecl) {
    $text = $ParamDecl.Trim()
    $text = $text -replace '^(INOUT|IN|OUT)\s+', ''
    $text = $text -replace '\bconst\b\s+', ''
    if ($text -match '\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)') {
        return $Matches[1]
    }
    if ($text -match '([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])?\s*$') {
        return $Matches[1]
    }
    return $text
}

function Get-ParamDirection([string]$ParamDecl, [string]$DocDirection) {
    if (-not [string]::IsNullOrWhiteSpace($DocDirection)) {
        switch -Regex ($DocDirection.ToLower()) {
            'inout|in/out' { return "in/out" }
            'out' { return "out" }
            default { return "in" }
        }
    }
    if ($ParamDecl -match '^\s*INOUT\b') {
        return "in/out"
    }
    if ($ParamDecl -match '^\s*OUT\b') {
        return "out"
    }
    if ($ParamDecl -match '^\s*IN\b') {
        return "in"
    }
    return "in"
}

function Get-StructNamesFromText([string]$Text) {
    $names = New-Object System.Collections.Generic.List[string]
    if ($null -eq $Text) {
        return $names.ToArray()
    }
    foreach ($match in [regex]::Matches($Text, 'NET_TV_[A-Za-z0-9_]+_S(?![A-Za-z0-9_])')) {
        $names.Add($match.Value)
    }
    return @($names.ToArray() | Select-Object -Unique)
}

function ConvertTo-LinkedText([string]$Text) {
    $escaped = ConvertTo-HtmlText $Text
    $escaped = [regex]::Replace($escaped, 'NET_TV_[A-Za-z0-9_]+_S(?![A-Za-z0-9_])', {
        param($match)
        $name = $match.Value
        if ($script:StructTopicNameSet.ContainsKey($name)) {
            return "<a href=`"$name.html`">$name</a>"
        }
        return $name
    })
    $escaped = [regex]::Replace($escaped, '\bNET_TV[A-Za-z0-9_]*\b', {
        param($match)
        $name = $match.Value
        if ($script:InterfaceTopicNameSet.ContainsKey($name)) {
            return "<a href=`"$name.html`">$name</a>"
        }
        return $name
    })
    return $escaped
}

function ConvertTo-LinkedCodeText([string]$Text, [bool]$LinkStructNames) {
    $escaped = ConvertTo-HtmlCodeText $Text
    if (-not $LinkStructNames) {
        return $escaped
    }

    return [regex]::Replace($escaped, 'NET_TV_[A-Za-z0-9_]+_S(?![A-Za-z0-9_])', {
        param($match)
        $name = $match.Value
        if ($script:StructTopicNameSet.ContainsKey($name)) {
            return "<a href=`"$name.html`">$name</a>"
        }
        return $name
    })
}

function ConvertTo-CodeBlockHtml([string]$Code, [bool]$LinkStructNames = $false) {
    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('  <div class="code-block" style="background-color: #e6e6e6; border: 0; padding: 8px 10px; margin: 12px 0 14px; font-family: &quot;Microsoft YaHei&quot;, &quot;微软雅黑&quot;, Arial, sans-serif; font-size: 10pt; line-height: 1.45;">')
    foreach ($line in ([regex]::Split($Code, '\r?\n'))) {
        $lineHtml = ConvertTo-LinkedCodeText $line $LinkStructNames
        if ([string]::IsNullOrEmpty($lineHtml)) {
            $lineHtml = '&nbsp;'
        }
        [void]$body.AppendLine("    <div class=""code-line"" style=""margin: 0; padding: 0; min-height: 1.45em; white-space: pre-wrap; background-color: #e6e6e6; font-family: &quot;Microsoft YaHei&quot;, &quot;微软雅黑&quot;, Arial, sans-serif; font-size: 10pt; line-height: 1.45;"">$lineHtml</div>")
    }
    [void]$body.AppendLine('  </div>')
    return $body.ToString()
}

function Add-CommonStyle($Body) {
    [void]$Body.AppendLine('  <style>')
    [void]$Body.AppendLine('    body { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; color: #000; }')
    [void]$Body.AppendLine('    h1 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 0 0 8px; }')
    [void]$Body.AppendLine('    h2 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 18px 0 6px; }')
    [void]$Body.AppendLine('    p { margin: 0 0 6px; }')
    [void]$Body.AppendLine('    table { border-collapse: collapse; margin-top: 10px; width: 1000px; table-layout: fixed; }')
    [void]$Body.AppendLine('    th, td { border: 1px solid #000; padding: 5px 8px; height: 30px; min-height: 30px; vertical-align: middle; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$Body.AppendLine('    th { background: #d9eaf7; font-weight: bold; }')
    [void]$Body.AppendLine('    .code-block { background-color: #e6e6e6; border: 0; padding: 8px 10px; margin: 12px 0 14px; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$Body.AppendLine('    .code-line { margin: 0; padding: 0; min-height: 1.45em; white-space: pre-wrap; background-color: #e6e6e6; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$Body.AppendLine('    .code-line a { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$Body.AppendLine('    .param-name { margin-left: 0; }')
    [void]$Body.AppendLine('    .param-desc { margin-left: 24px; }')
    [void]$Body.AppendLine('  </style>')
}

function New-HtmlStart([string]$Title) {
    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('<!doctype html>')
    [void]$body.AppendLine('<html>')
    [void]$body.AppendLine('<head>')
    [void]$body.AppendLine('  <meta charset="utf-8">')
    [void]$body.AppendLine("  <title>$(ConvertTo-HtmlText $Title)</title>")
    Add-CommonStyle $body
    [void]$body.AppendLine('</head>')
    [void]$body.AppendLine('<body>')
    [void]$body.AppendLine("  <h1>$(ConvertTo-HtmlText $Title)</h1>")
    return $body
}

function Complete-Html($Body) {
    [void]$Body.AppendLine('</body>')
    [void]$Body.AppendLine('</html>')
    return $Body.ToString()
}

function ConvertTo-DocLines([string]$Comment) {
    $lines = New-Object System.Collections.Generic.List[string]
    if ([string]::IsNullOrWhiteSpace($Comment)) {
        return $lines.ToArray()
    }

    foreach ($raw in ([regex]::Split($Comment, '\r?\n'))) {
        $line = $raw.Trim()
        $line = $line -replace '^/\*\*', ''
        $line = $line -replace '\*/$', ''
        $line = $line -replace '^\*', ''
        $line = $line.Trim()
        if (-not [string]::IsNullOrWhiteSpace($line)) {
            $lines.Add($line)
        }
    }
    return $lines.ToArray()
}

function ConvertTo-ReadableText([string]$Text) {
    $value = CleanInline $Text
    if ([string]::IsNullOrWhiteSpace($value)) {
        return ""
    }

    $value = $value -replace 'TRUE表示成功,其他表示失败\s+TRUE means success, and any other value means failure\.?', 'TRUE表示成功，FALSE/其他值表示失败'
    $value = $value -replace 'TRUE 成功，FALSE 失败（调用 NET_TV_GetLastError\(\) 获取错误码）', 'TRUE表示成功，FALSE表示失败，可调用NET_TV_GetLastError获取错误码'
    $value = $value -replace 'TRUE 成功，FALSE 失败；失败原因通过 NET_TV_GetLastError 获取', 'TRUE表示成功，FALSE表示失败，可调用NET_TV_GetLastError获取错误码'
    $value = $value -replace '\s+SDK version information$', ''
    $value = $value -replace '\s+Get SDK version information$', ''
    $value = $value -replace '\s+Get error codes$', ''
    $value = $value -replace '\s+User login ID$', ''
    $value = $value -replace '\s+Device configuration commands.*$', ''
    $value = $value -replace '\s+Pointer to .*$', ''
    $value = $value -replace '\s+Length \(in byte\).*$',''
    $value = $value -replace '\s+', ' '
    return $value.Trim()
}

function Parse-DocComment([string]$Comment) {
    $brief = ""
    $returnText = ""
    $noteLines = New-Object System.Collections.Generic.List[string]
    $params = @{}
    $currentTag = ""
    $currentParam = ""

    foreach ($lineRaw in (ConvertTo-DocLines $Comment)) {
        $line = $lineRaw.Trim()
        if ($line -match '^@brief\s+(.+)$') {
            $brief = ConvertTo-ReadableText $Matches[1]
            $currentTag = "brief"
            $currentParam = ""
            continue
        }
        if ($line -match '^@param\s*(?:\[([A-Za-z/]+)\])?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(.*)$') {
            $dir = $Matches[1]
            $name = $Matches[2]
            $desc = ConvertTo-ReadableText $Matches[3]
            $params[$name] = [pscustomobject]@{
                Direction = $dir
                Description = $desc
            }
            $currentTag = "param"
            $currentParam = $name
            continue
        }
        if ($line -match '^@return\s+(.+)$') {
            $returnText = ConvertTo-ReadableText $Matches[1]
            $currentTag = "return"
            $currentParam = ""
            continue
        }
        if ($line -match '^@note\s*(.*)$') {
            $note = ConvertTo-ReadableText $Matches[1]
            if (-not [string]::IsNullOrWhiteSpace($note)) {
                $noteLines.Add($note)
            }
            $currentTag = "note"
            $currentParam = ""
            continue
        }
        if ($line -match '^-+\s*(.+)$') {
            $noteLines.Add((ConvertTo-ReadableText $Matches[1]))
            continue
        }

        $append = ConvertTo-ReadableText $line
        if ([string]::IsNullOrWhiteSpace($append)) {
            continue
        }
        if ([string]::IsNullOrWhiteSpace($brief) -and $line -notmatch '^@') {
            $brief = $append
            $currentTag = "brief"
            continue
        }
        if ($currentTag -eq "param" -and -not [string]::IsNullOrWhiteSpace($currentParam) -and $params.ContainsKey($currentParam)) {
            $params[$currentParam].Description = ($params[$currentParam].Description + " " + $append).Trim()
        } elseif ($currentTag -eq "return") {
            $returnText = ($returnText + " " + $append).Trim()
        } elseif ($currentTag -eq "note") {
            $noteLines.Add($append)
        }
    }

    return [pscustomobject]@{
        Brief = $brief
        Params = $params
        ReturnText = $returnText
        Notes = $noteLines.ToArray()
    }
}

function Normalize-Prototype([string]$Prototype) {
    $value = [regex]::Replace($Prototype, '\s+', ' ').Trim()
    $value = $value -replace '^NET_TV_API\s+', ''
    $value = $value -replace '\s+STDCALL\s+', ' '
    $value = $value -replace '\(\s+', '('
    $value = $value -replace '\s+\)', ')'
    $value = $value.Trim()
    if ($value.EndsWith(';')) {
        $value = $value.Substring(0, $value.Length - 1).Trim()
    }

    if ($value -match '^(.*?)\s+(NET_TV[A-Za-z0-9_]+)\s*\((.*)\)$') {
        $returnType = $Matches[1].Trim()
        $name = $Matches[2].Trim()
        $params = @(Split-ParameterList $Matches[3])
        if ($params.Count -eq 0) {
            return "$returnType $name(void);"
        }

        $sb = New-Object System.Text.StringBuilder
        [void]$sb.AppendLine("$returnType $name(")
        for ($i = 0; $i -lt $params.Count; $i++) {
            $suffix = if ($i -lt ($params.Count - 1)) { "," } else { "" }
            [void]$sb.AppendLine("    $($params[$i])$suffix")
        }
        [void]$sb.Append(');')
        return $sb.ToString()
    }

    return "$value;"
}

function Get-PrototypeParams([string]$Prototype) {
    $value = [regex]::Replace($Prototype, '\s+', ' ').Trim()
    $value = $value -replace '^NET_TV_API\s+', ''
    $value = $value -replace '\s+STDCALL\s+', ' '
    if ($value -match '\((.*)\)\s*;?\s*$') {
        return @(Split-ParameterList $Matches[1])
    }
    return @()
}

function Get-InterfaceCategory([string]$Name, [string]$Description) {
    if ($Name -match 'Init|Cleanup|SDKVersion|LastError|Log|RevTimeOut|ConnectTime|ExceptionCallBack') {
        return "SDK基础"
    }
    if ($Name -match 'Login|Logout') {
        return "登录设备"
    }
    if ($Name -match 'Alarm|Listen|ChannelStatus') {
        return "报警监听"
    }
    if ($Name -match 'Capability') {
        return "设备能力"
    }
    if ($Name -match 'DevConfig|DeviceControl|Upload|Upgrade') {
        return "配置/控制/升级"
    }
    if ($Name -match 'Discovery') {
        return "设备发现"
    }
    if ($Name -match 'VoiceCom') {
        return "语音对讲"
    }
    if ($Name -match 'Replay|RecordFrame') {
        return "回放/录像"
    }
    return "通用接口"
}

function Get-ApiFunctions([string]$HeaderPath) {
    $content = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $HeaderPath).Path, [System.Text.Encoding]::UTF8)
    $items = New-Object System.Collections.Generic.List[object]

    $pattern = '(?s)(?<comment>/\*\*.*?\*/)\s*(?<proto>NET_TV_API\s+.*?\bNET_TV[A-Za-z0-9_]+\s*\(.*?\)\s*;)'
    foreach ($match in [regex]::Matches($content, $pattern)) {
        $comment = $match.Groups['comment'].Value
        $proto = $match.Groups['proto'].Value
        if ($proto -notmatch '\b(NET_TV[A-Za-z0-9_]+)\s*\(') {
            continue
        }

        $name = $Matches[1]
        if ($name -eq 'NET_TV_API') {
            continue
        }

        $doc = Parse-DocComment $comment
        $description = $doc.Brief
        if ([string]::IsNullOrWhiteSpace($description)) {
            $description = $name
        }
        $params = @(Get-PrototypeParams $proto)
        $relatedStructs = @(Get-StructNamesFromText $proto)

        $items.Add([pscustomobject]@{
            Name = $name
            Category = Get-InterfaceCategory $name $description
            Description = $description
            Prototype = $proto
            Signature = Normalize-Prototype $proto
            Params = $params
            ParamDocs = $doc.Params
            ReturnText = $doc.ReturnText
            Notes = $doc.Notes
            RelatedStructs = $relatedStructs
        })
    }

    return @($items.ToArray() | Sort-Object Name)
}

function Get-FieldDescription([string]$Line) {
    if ($Line -match '/\*\s*(.*?)\s*\*/') {
        return ConvertTo-ReadableText $Matches[1]
    }
    if ($Line -match '//\s*(.+)$') {
        return ConvertTo-ReadableText $Matches[1]
    }
    return ""
}

function Get-FieldDeclaration([string]$Line) {
    $value = $Line.Trim()
    $value = $value -replace '/\*.*?\*/', ''
    $value = $value -replace '//.*$', ''
    $value = $value.Trim()
    if ($value.EndsWith(';')) {
        $value = $value.Substring(0, $value.Length - 1).Trim()
    }
    return $value
}

function Get-StructDefinitions([string]$HeaderPath) {
    $content = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $HeaderPath).Path, [System.Text.Encoding]::UTF8)
    $items = New-Object System.Collections.Generic.List[object]

    $pattern = '(?s)(?<comment>/\*\*.*?\*/\s*)?typedef\s+struct\s+(?<tag>[A-Za-z0-9_]*)\s*\{(?<body>.*?)\}\s*(?<name>NET_TV[A-Za-z0-9_]+_S)\s*(?<alias>,\s*\*?[A-Za-z0-9_]+)?\s*;'
    foreach ($match in [regex]::Matches($content, $pattern)) {
        $name = $match.Groups['name'].Value
        $comment = $match.Groups['comment'].Value
        $doc = Parse-DocComment $comment
        $description = $doc.Brief
        if ([string]::IsNullOrWhiteSpace($description)) {
            $description = "接口参数结构体"
        }

        $fields = New-Object System.Collections.Generic.List[object]
        foreach ($rawLine in ([regex]::Split($match.Groups['body'].Value, '\r?\n'))) {
            $line = $rawLine.Trim()
            if ([string]::IsNullOrWhiteSpace($line) -or $line.StartsWith('/*') -or $line.StartsWith('*') -or $line.StartsWith('//')) {
                continue
            }
            if ($line -notmatch ';') {
                continue
            }
            $decl = Get-FieldDeclaration $line
            if ([string]::IsNullOrWhiteSpace($decl)) {
                continue
            }
            $desc = Get-FieldDescription $line
            if ([string]::IsNullOrWhiteSpace($desc)) {
                $fieldName = Get-ParamName $decl
                if ($fieldName -match '^byRes') {
                    $desc = "保留字段"
                } else {
                    $desc = "字段"
                }
            }

            $fields.Add([pscustomobject]@{
                Declaration = $decl
                Name = Get-ParamName $decl
                Direction = "in"
                Description = $desc
            })
        }

        $definition = $match.Value.Trim()
        $definition = [regex]::Replace($definition, '\r\n|\r|\n', "`n")
        $items.Add([pscustomobject]@{
            Title = $name
            Description = $description
            Definition = $definition
            Fields = $fields.ToArray()
        })
    }

    return @($items.ToArray() | Sort-Object Title)
}

function New-InterfaceIndexHtml($Interfaces) {
    $body = New-HtmlStart $InterfaceTopicTitle
    [void]$body.AppendLine('  <p>本章节列出 SDK client 对外提供的接口，接口详情 topic 中包含函数原型、参数说明、返回值和关联结构体。</p>')
    [void]$body.AppendLine('  <table>')
    [void]$body.AppendLine('    <tr><th>类型</th><th>功能描述</th><th>接口</th><th>相关参数</th></tr>')
    foreach ($api in $Interfaces) {
        $related = if ($api.RelatedStructs.Count -gt 0) {
            (($api.RelatedStructs | ForEach-Object {
                if ($script:StructTopicNameSet.ContainsKey($_)) {
                    "<a href=`"$_.html`">$_</a>"
                } else {
                    ConvertTo-HtmlText $_
                }
            }) -join '、')
        } else {
            "无"
        }
        [void]$body.AppendLine("    <tr><td>$(ConvertTo-HtmlText $api.Category)</td><td>$(ConvertTo-HtmlText $api.Description)</td><td><a href=""$($api.Name).html"">$($api.Name)</a></td><td>$related</td></tr>")
    }
    [void]$body.AppendLine('  </table>')
    return Complete-Html $body
}

function New-InterfaceDetailHtml($Api, $DemoRefsByInterface) {
    $body = New-HtmlStart $Api.Name
    [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $Api.Description)</p>")
    [void]$body.Append((ConvertTo-CodeBlockHtml $Api.Signature $true))

    [void]$body.AppendLine('  <h2>Parameters</h2>')
    if ($Api.Params.Count -eq 0) {
        [void]$body.AppendLine('  <p>无参数。</p>')
    } else {
        foreach ($paramDecl in $Api.Params) {
            $paramName = Get-ParamName $paramDecl
            $docDir = ""
            $desc = ""
            if ($Api.ParamDocs.ContainsKey($paramName)) {
                $docDir = $Api.ParamDocs[$paramName].Direction
                $desc = $Api.ParamDocs[$paramName].Description
            }
            if ([string]::IsNullOrWhiteSpace($desc)) {
                if ($paramDecl -match 'NET_TV_[A-Za-z0-9_]+_S') {
                    $desc = "接口关联结构体数据"
                } elseif ($paramName -match 'Callback|cb|CB') {
                    $desc = "回调函数指针"
                } else {
                    $desc = "参数"
                }
            }
            $direction = Get-ParamDirection $paramDecl $docDir
            [void]$body.AppendLine("  <p class=""param-name"">[$direction] $(ConvertTo-HtmlText $paramName)</p>")
            [void]$body.AppendLine("  <p class=""param-desc"">$(ConvertTo-LinkedText $desc)</p>")
        }
    }

    [void]$body.AppendLine('  <h2>Return Values</h2>')
    if ([string]::IsNullOrWhiteSpace($Api.ReturnText)) {
        [void]$body.AppendLine('  <p>接口返回值请以函数声明和运行时错误码为准，失败时可调用 NET_TV_GetLastError 获取错误码。</p>')
    } else {
        [void]$body.AppendLine("  <p>$(ConvertTo-LinkedText $Api.ReturnText)</p>")
    }

    [void]$body.AppendLine('  <h2>Remarks</h2>')
    if ($Api.Notes.Count -gt 0) {
        foreach ($note in $Api.Notes) {
            [void]$body.AppendLine("  <p>$(ConvertTo-LinkedText $note)</p>")
        }
    } else {
        [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $Api.Description)</p>")
    }

    [void]$body.AppendLine('  <h2>See Also</h2>')
    $hasSeeAlso = $false
    foreach ($structName in $Api.RelatedStructs) {
        if ($script:StructTopicNameSet.ContainsKey($structName)) {
            [void]$body.AppendLine("  <p><a href=""$structName.html"">$structName</a></p>")
            $hasSeeAlso = $true
        }
    }
    if ($DemoRefsByInterface.ContainsKey($Api.Name)) {
        foreach ($demo in $DemoRefsByInterface[$Api.Name]) {
            [void]$body.AppendLine("  <p><a href=""$($demo.FileName)"">$($demo.Title)</a></p>")
            $hasSeeAlso = $true
        }
    }
    if (-not $hasSeeAlso) {
        [void]$body.AppendLine('  <p>无。</p>')
    }

    return Complete-Html $body
}

function New-StructDetailHtml($StructInfo) {
    $body = New-HtmlStart $StructInfo.Title
    [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $StructInfo.Description)</p>")
    [void]$body.Append((ConvertTo-CodeBlockHtml $StructInfo.Definition $true))

    [void]$body.AppendLine('  <h2>Parameters</h2>')
    if ($StructInfo.Fields.Count -eq 0) {
        [void]$body.AppendLine('  <p>无字段。</p>')
    } else {
        foreach ($field in $StructInfo.Fields) {
            [void]$body.AppendLine("  <p class=""param-name"">[$($field.Direction)] $(ConvertTo-HtmlText $field.Name)</p>")
            [void]$body.AppendLine("  <p class=""param-desc"">$(ConvertTo-LinkedText $field.Description)</p>")
        }
    }

    [void]$body.AppendLine('  <h2>See Also</h2>')
    $seeAlso = New-Object System.Collections.Generic.List[string]
    foreach ($field in $StructInfo.Fields) {
        foreach ($structName in @(Get-StructNamesFromText $field.Declaration)) {
            if ($structName -ne $StructInfo.Title -and $script:StructTopicNameSet.ContainsKey($structName)) {
                $seeAlso.Add($structName)
            }
        }
    }
    $seeAlsoUnique = @($seeAlso.ToArray() | Select-Object -Unique)
    if ($seeAlsoUnique.Count -eq 0) {
        [void]$body.AppendLine('  <p>无。</p>')
    } else {
        foreach ($structName in $seeAlsoUnique) {
            [void]$body.AppendLine("  <p><a href=""$structName.html"">$structName</a></p>")
        }
    }

    return Complete-Html $body
}

function Get-DemoDescription([string]$RelativePath) {
    switch -Regex ($RelativePath) {
        'alarm' { return "报警监听与告警图片保存调用示例" }
        'capability' { return "设备能力获取调用示例" }
        'config' { return "设备配置获取、设置和升级调用示例" }
        'discovery' { return "局域网设备搜索调用示例" }
        'http_face' { return "HTTP/MQTT 人脸相关接口调用示例" }
        default { return "SDK client接口调用示例" }
    }
}

function Get-DemoTopics([string]$DemoRoot, $Interfaces) {
    $root = (Resolve-Path -LiteralPath $DemoRoot).Path
    $interfaceNames = @($Interfaces | ForEach-Object { $_.Name })
    $items = New-Object System.Collections.Generic.List[object]

    $files = @(Get-ChildItem -LiteralPath $root -Recurse -File |
        Where-Object { $_.Extension -in @('.c', '.cpp', '.h') } |
        Sort-Object FullName)
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($root.Length).TrimStart('\', '/')
        $title = "Demo_$($relative -replace '[\\/:*?""<>|\. ]', '_')"
        $content = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
        $usedInterfaces = New-Object System.Collections.Generic.List[string]
        foreach ($name in $interfaceNames) {
            if ($content -match [regex]::Escape($name)) {
                $usedInterfaces.Add($name)
            }
        }

        $items.Add([pscustomobject]@{
            Title = $title
            SourcePath = $relative
            Description = Get-DemoDescription $relative
            Content = $content
            Interfaces = @($usedInterfaces.ToArray() | Sort-Object -Unique)
        })
    }

    return $items.ToArray()
}

function New-DemoIndexHtml($Demos) {
    $body = New-HtmlStart $DemoTopicTitle
    [void]$body.AppendLine('  <p>本章节按 SDK client demo 源文件生成调用示例 topic，便于从接口定义跳转到实际调用代码。</p>')
    [void]$body.AppendLine('  <table>')
    [void]$body.AppendLine('    <tr><th>Demo</th><th>功能说明</th><th>源文件</th><th>相关接口</th></tr>')
    foreach ($demo in $Demos) {
        $links = if ($demo.Interfaces.Count -gt 0) {
            (($demo.Interfaces | ForEach-Object { "<a href=`"$_.html`">$_</a>" }) -join '、')
        } else {
            "无"
        }
        [void]$body.AppendLine("    <tr><td><a href=""$($demo.FileName)"">$($demo.Title)</a></td><td>$(ConvertTo-HtmlText $demo.Description)</td><td>$(ConvertTo-HtmlText $demo.SourcePath)</td><td>$links</td></tr>")
    }
    [void]$body.AppendLine('  </table>')
    return Complete-Html $body
}

function New-DemoDetailHtml($Demo) {
    $body = New-HtmlStart $Demo.Title
    [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $Demo.Description)</p>")
    [void]$body.AppendLine("  <p>源文件：$(ConvertTo-HtmlText $Demo.SourcePath)</p>")
    if ($Demo.Interfaces.Count -gt 0) {
        [void]$body.AppendLine('  <h2>相关接口</h2>')
        foreach ($name in $Demo.Interfaces) {
            [void]$body.AppendLine("  <p><a href=""$name.html"">$name</a></p>")
        }
    }
    [void]$body.AppendLine('  <h2>Demo Code</h2>')
    [void]$body.Append((ConvertTo-CodeBlockHtml $Demo.Content $true))
    return Complete-Html $body
}

function Add-HtmlTopic($List, [string]$Section, [string]$Title, [string]$ParentSection, [string]$Html, [string]$FileName = "") {
    if ([string]::IsNullOrWhiteSpace($FileName)) {
        $FileName = (ConvertTo-SafeFileName $Title) + ".html"
    }
    $List.Add([pscustomobject]@{
        Section = $Section
        ParentSection = $ParentSection
        Title = $Title
        HelpId = ConvertTo-HelpId $Title
        FileName = $FileName
        Html = $Html
    })
}

function Write-TopicFile($Topic, [string]$Dir) {
    $path = Join-Path $Dir $Topic.FileName
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($path, $Topic.Html, $encoding)
}

function New-ImportScript($Topics, [string]$OutputDir) {
    $fullDir = (Resolve-Path -LiteralPath $OutputDir).Path
    $interfaceIndex = @($Topics | Where-Object { $_.Section -eq 'interface_index' })
    $interfaces = @($Topics | Where-Object { $_.Section -eq 'interface_detail' })
    $structs = @($Topics | Where-Object { $_.Section -eq 'struct' })
    $demoIndex = @($Topics | Where-Object { $_.Section -eq 'demo_index' })
    $demos = @($Topics | Where-Object { $_.Section -eq 'demo_detail' })

    $script = New-Object System.Text.StringBuilder
    [void]$script.AppendLine('// HelpNDoc Pascal Script')
    [void]$script.AppendLine('// 在 HelpNDoc 中打开目标 .hnd 工程后，运行本脚本。')
    [void]$script.AppendLine('// 作用：自动创建 SDK client 接口定义、接口参数结构体和接口调用Demo topic，并导入对应 HTML 内容。')
    [void]$script.AppendLine('')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  RootTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  InterfaceTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  StructTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  DemoTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  NewTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  Editor: TObject;')
    [void]$script.AppendLine('')
    [void]$script.AppendLine('begin')
    [void]$script.AppendLine('  Editor := HndEditor.CreateTemporaryEditor();')
    [void]$script.AppendLine('  try')
    [void]$script.AppendLine('    RootTopic := HndTopics.CreateTopic();')
    [void]$script.AppendLine("    HndTopics.SetTopicCaption(RootTopic, '$(Escape-Pascal $RootTopicTitle)');")
    [void]$script.AppendLine("    HndTopics.SetTopicHelpId(RootTopic, '$(Escape-Pascal (ConvertTo-HelpId $RootTopicTitle))');")
    [void]$script.AppendLine('')

    if ($interfaceIndex.Count -gt 0) {
        $topic = $interfaceIndex[0]
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine('    InterfaceTopic := HndTopics.CreateTopic();')
        [void]$script.AppendLine("    HndTopics.SetTopicCaption(InterfaceTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("    HndTopics.SetTopicHelpId(InterfaceTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine('    HndTopics.MoveTopic(InterfaceTopic, RootTopic, htamAddChild);')
        [void]$script.AppendLine('    HndEditor.Clear(Editor);')
        [void]$script.AppendLine("    HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('    HndEditor.SetAsTopicContent(Editor, InterfaceTopic);')
        [void]$script.AppendLine('')
    }

    foreach ($topic in $interfaces) {
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine('    NewTopic := HndTopics.CreateTopic();')
        [void]$script.AppendLine("    HndTopics.SetTopicCaption(NewTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("    HndTopics.SetTopicHelpId(NewTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine('    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);')
        [void]$script.AppendLine('    HndEditor.Clear(Editor);')
        [void]$script.AppendLine("    HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('    HndEditor.SetAsTopicContent(Editor, NewTopic);')
        [void]$script.AppendLine('')
    }

    [void]$script.AppendLine('    StructTopic := HndTopics.CreateTopic();')
    [void]$script.AppendLine("    HndTopics.SetTopicCaption(StructTopic, '$(Escape-Pascal $StructTopicTitle)');")
    [void]$script.AppendLine("    HndTopics.SetTopicHelpId(StructTopic, '$(Escape-Pascal (ConvertTo-HelpId $StructTopicTitle))');")
    [void]$script.AppendLine('    HndTopics.MoveTopic(StructTopic, RootTopic, htamAddChild);')
    [void]$script.AppendLine('')
    foreach ($topic in $structs) {
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine('    NewTopic := HndTopics.CreateTopic();')
        [void]$script.AppendLine("    HndTopics.SetTopicCaption(NewTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("    HndTopics.SetTopicHelpId(NewTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine('    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);')
        [void]$script.AppendLine('    HndEditor.Clear(Editor);')
        [void]$script.AppendLine("    HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('    HndEditor.SetAsTopicContent(Editor, NewTopic);')
        [void]$script.AppendLine('')
    }

    if ($demoIndex.Count -gt 0) {
        $topic = $demoIndex[0]
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine('    DemoTopic := HndTopics.CreateTopic();')
        [void]$script.AppendLine("    HndTopics.SetTopicCaption(DemoTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("    HndTopics.SetTopicHelpId(DemoTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine('    HndTopics.MoveTopic(DemoTopic, RootTopic, htamAddChild);')
        [void]$script.AppendLine('    HndEditor.Clear(Editor);')
        [void]$script.AppendLine("    HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('    HndEditor.SetAsTopicContent(Editor, DemoTopic);')
        [void]$script.AppendLine('')
    }

    foreach ($topic in $demos) {
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine('    NewTopic := HndTopics.CreateTopic();')
        [void]$script.AppendLine("    HndTopics.SetTopicCaption(NewTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("    HndTopics.SetTopicHelpId(NewTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine('    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);')
        [void]$script.AppendLine('    HndEditor.Clear(Editor);')
        [void]$script.AppendLine("    HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('    HndEditor.SetAsTopicContent(Editor, NewTopic);')
        [void]$script.AppendLine('')
    }

    [void]$script.AppendLine('  finally')
    [void]$script.AppendLine('    HndEditor.DestroyTemporaryEditor(Editor);')
    [void]$script.AppendLine('  end;')
    [void]$script.AppendLine('end.')
    return $script.ToString()
}

$outputFullPath = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDir))
if (Test-Path -LiteralPath $outputFullPath) {
    Remove-Item -LiteralPath $outputFullPath -Recurse -Force
}
New-Item -ItemType Directory -Path $outputFullPath -Force | Out-Null

$interfaces = @(Get-ApiFunctions $ClientHeaderPath)
$structs = @(Get-StructDefinitions $CommonHeaderPath)

foreach ($struct in $structs) {
    $script:StructTopicNameSet[$struct.Title] = $true
}
foreach ($api in $interfaces) {
    $script:InterfaceTopicNameSet[$api.Name] = $true
}

$demoInfos = @(Get-DemoTopics $DemoDir $interfaces)
$demoRefsByInterface = @{}

$topics = New-Object System.Collections.Generic.List[object]

foreach ($demo in $demoInfos) {
    $demoFileName = (ConvertTo-SafeFileName $demo.Title) + ".html"
    $demo | Add-Member -NotePropertyName FileName -NotePropertyValue $demoFileName
    foreach ($interfaceName in $demo.Interfaces) {
        if (-not $demoRefsByInterface.ContainsKey($interfaceName)) {
            $demoRefsByInterface[$interfaceName] = New-Object System.Collections.Generic.List[object]
        }
        $demoRefsByInterface[$interfaceName].Add($demo)
    }
}

Add-HtmlTopic $topics 'interface_index' $InterfaceTopicTitle '' (New-InterfaceIndexHtml $interfaces)
foreach ($api in $interfaces) {
    Add-HtmlTopic $topics 'interface_detail' $api.Name 'interface_index' (New-InterfaceDetailHtml $api $demoRefsByInterface)
}

foreach ($struct in $structs) {
    Add-HtmlTopic $topics 'struct' $struct.Title 'struct_index' (New-StructDetailHtml $struct)
}

Add-HtmlTopic $topics 'demo_index' $DemoTopicTitle '' (New-DemoIndexHtml $demoInfos)
foreach ($demo in $demoInfos) {
    Add-HtmlTopic $topics 'demo_detail' $demo.Title 'demo_index' (New-DemoDetailHtml $demo) $demo.FileName
}

$topics = @($topics | Sort-Object @{ Expression = {
    switch ($_.Section) {
        'interface_index' { 0 }
        'interface_detail' { 1 }
        'struct' { 2 }
        'demo_index' { 3 }
        'demo_detail' { 4 }
        default { 9 }
    }
}}, Title)

foreach ($topic in $topics) {
    Write-TopicFile $topic $outputFullPath
}

$manifestPath = Join-Path $outputFullPath "topics_manifest.csv"
$topics | Select-Object Section, ParentSection, Title, HelpId, FileName | Export-Csv -LiteralPath $manifestPath -NoTypeInformation -Encoding UTF8

$scriptPath = Join-Path $outputFullPath "import_to_helpndoc.pas"
$importScript = New-ImportScript $topics $outputFullPath
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($scriptPath, $importScript, $utf8NoBom)

Write-Host "Output directory: $outputFullPath"
Write-Host "Topic files: $($topics.Count)"
Write-Host "HelpNDoc import script: $scriptPath"
Write-Host "Manifest: $manifestPath"
