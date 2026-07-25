param(
    [string]$MarkdownPath = "docs/sdk-ipc-tvsdk-interface-reference.md",
    [string]$InterfaceHeaderPath = "SDK/af_sdk/sdk_server/include/NetTVSDKServerInterface.h",
    [string]$CommonHeaderPath = "SDK/af_sdk/sdk_share/include/NetTVSDKCommon.h",
    [string]$OutputDir = "docs/helpndoc_tvsdk_topics",
    [string]$RootTopicTitle = "SDK server接口定义",
    [string]$CallbackTopicTitle = "接口回调",
    [string]$StructTopicTitle = "接口参数结构体",
    [switch]$IncludeCallbackTopic
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$script:StructTopicNameSet = @{}
$script:CallbackInterfaceNameSet = @{}

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
    return $value
}

function ConvertTo-HelpId([string]$Title) {
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
    $name = ($Title -replace '[\\/:*?"<>|]', '_')
    $name = $name.Trim()
    if ($name.Length -gt 120) {
        $name = $name.Substring(0, 120)
    }
    return $name
}

function ConvertTo-PascalIdentifier([string]$Title) {
    $name = ($Title -replace '[^A-Za-z0-9_]', '_').Trim('_')
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = "Topic"
    }
    return "Topic_$name"
}

function Split-MarkdownTableRow([string]$Line) {
    $text = $Line.Trim()
    if ($text.StartsWith("|")) {
        $text = $text.Substring(1)
    }
    if ($text.EndsWith("|")) {
        $text = $text.Substring(0, $text.Length - 1)
    }

    $cells = New-Object System.Collections.Generic.List[string]
    $sb = New-Object System.Text.StringBuilder
    $escaped = $false
    foreach ($ch in $text.ToCharArray()) {
        if ($escaped) {
            [void]$sb.Append($ch)
            $escaped = $false
            continue
        }
        if ($ch -eq '\') {
            $escaped = $true
            [void]$sb.Append($ch)
            continue
        }
        if ($ch -eq '|') {
            $cells.Add($sb.ToString().Trim())
            [void]$sb.Clear()
            continue
        }
        [void]$sb.Append($ch)
    }
    $cells.Add($sb.ToString().Trim())
    return $cells.ToArray()
}

function Test-TableSeparator([string]$Line) {
    return ($Line.Trim() -match '^\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)+\|?\s*$')
}

function Remove-StatusFromParamText([string]$Text) {
    if ($null -eq $Text) {
        return ""
    }
    $value = CleanInline $Text
    $value = $value -replace '\s*；\s*状态\s*:\s*.*$', ''
    $value = $value -replace '\s*;\s*状态\s*:\s*.*$', ''
    $value = $value -replace '\s*状态\s*:\s*.*$', ''
    return $value.Trim()
}

function Get-BacktickValues([string]$Text) {
    $values = New-Object System.Collections.Generic.List[string]
    if ($null -eq $Text) {
        return $values.ToArray()
    }
    foreach ($match in [regex]::Matches($Text, '`([^`]+)`')) {
        $values.Add($match.Groups[1].Value)
    }
    return $values.ToArray()
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
    $text = $text -replace '^(IN|OUT|INOUT)\s+', ''
    if ($text -match '\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)') {
        return $Matches[1]
    }
    if ($text -match '([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])?\s*$') {
        return $Matches[1]
    }
    return $text
}

function Get-ParamDirection([string]$ParamDecl) {
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

function Get-ParamDescription([string]$ParamName, [string]$ParamDecl, [object]$CallbackRow) {
    $map = @{
        udwPort = "SDK服务端监听端口"
        szUserName = "SDK服务端鉴权用户名"
        szPassword = "SDK服务端鉴权密码"
        pAlarmer = "报警设备信息"
        lCommand = "告警命令码"
        pAlarmInfo = "告警信息结构体数据"
        dwBufLen = "告警信息数据长度"
        pChannelInfo = "通道状态信息"
        pCb = "业务回调函数指针"
        CB = "业务回调函数指针"
        pInfo = "设备信息输出参数"
        lpOutBuffer = "配置数据输出缓冲区"
        lpInBuffer = "配置数据输入缓冲区"
        dwChannelID = "通道号"
        strLogDir = "日志文件目录"
        dwLogLevel = "日志等级"
        dwLogFileSize = "单个日志文件大小"
        dwLogFileNum = "日志文件数量"
    }
    if ($map.ContainsKey($ParamName)) {
        return $map[$ParamName]
    }
    if ($ParamDecl -match 'NET_TV_[A-Za-z0-9_]+_S') {
        return "接口关联结构体数据"
    }
    if ($ParamName -match 'pCb|CB|Callback') {
        return "回调函数指针"
    }
    return "参数"
}

function ConvertTo-LinkedHtmlText([string]$Text, [bool]$LinkStructNames) {
    $escaped = ConvertTo-HtmlText $Text
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

function ConvertTo-LinkedCodeHtmlText([string]$Text, [bool]$LinkStructNames) {
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

function ConvertTo-LinkedInterfaceHtmlText([string]$Text) {
    $escaped = ConvertTo-LinkedHtmlText $Text $true
    return [regex]::Replace($escaped, 'NET_TV[A-Za-z0-9_]*|NET_TV_[A-Za-z0-9_]+', {
        param($match)
        $name = $match.Value
        if ($script:CallbackInterfaceNameSet.ContainsKey($name)) {
            return "<a href=`"$name.html`">$name</a>"
        }
        return $name
    })
}

function ConvertTo-CodeBlockHtml([string]$Code, [bool]$LinkStructNames) {
    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('  <div class="code-block" style="background-color: #e6e6e6; border: 0; padding: 8px 10px; margin: 12px 0 14px; font-family: &quot;Microsoft YaHei&quot;, &quot;微软雅黑&quot;, Arial, sans-serif; font-size: 10pt; line-height: 1.45;">')

    foreach ($line in ([regex]::Split($Code, '\r?\n'))) {
        $lineHtml = ConvertTo-LinkedCodeHtmlText $line $LinkStructNames
        if ([string]::IsNullOrEmpty($lineHtml)) {
            $lineHtml = '&nbsp;'
        }
        [void]$body.AppendLine("    <div class=""code-line"" style=""margin: 0; padding: 0; min-height: 1.45em; white-space: pre-wrap; background-color: #e6e6e6; font-family: &quot;Microsoft YaHei&quot;, &quot;微软雅黑&quot;, Arial, sans-serif; font-size: 10pt; line-height: 1.45;"">$lineHtml</div>")
    }

    [void]$body.AppendLine('  </div>')
    return $body.ToString()
}

function New-TopicHtml([string]$Title, [string[]]$Lines, [bool]$LinkStructNames = $false) {
    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('<!doctype html>')
    [void]$body.AppendLine('<html>')
    [void]$body.AppendLine('<head>')
    [void]$body.AppendLine('  <meta charset="utf-8">')
    [void]$body.AppendLine("  <title>$(ConvertTo-HtmlText $Title)</title>")
    [void]$body.AppendLine('  <style>')
    [void]$body.AppendLine('    body { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; color: #000; }')
    [void]$body.AppendLine('    h1 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 0 0 14px; }')
    [void]$body.AppendLine('    p { margin: 0 0 4px; }')
    [void]$body.AppendLine('    table { border-collapse: collapse; margin-top: 10px; width: 1000px; table-layout: fixed; }')
    [void]$body.AppendLine('    th, td { border: 1px solid #000; padding: 5px 8px; height: 30px; min-height: 30px; vertical-align: middle; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$body.AppendLine('    th { background: #d9eaf7; font-weight: bold; }')
    [void]$body.AppendLine('    code { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$body.AppendLine('  </style>')
    [void]$body.AppendLine('</head>')
    [void]$body.AppendLine('<body>')
    [void]$body.AppendLine("  <h1>$(ConvertTo-HtmlText $Title)</h1>")

    $inTable = $false
    foreach ($line in $Lines) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        if ($line -match '^###\s+') {
            continue
        }
        if (Test-TableSeparator $line) {
            continue
        }
        if ($line.Trim().StartsWith('|')) {
            $cells = Split-MarkdownTableRow $line
            if (-not $inTable) {
                [void]$body.AppendLine('  <table>')
                $inTable = $true
            }
            $tag = 'td'
            if ($cells.Count -gt 0 -and ($cells[0] -eq '字段声明' -or $cells[0] -eq '类型')) {
                $tag = 'th'
            }
            [void]$body.AppendLine('    <tr>')
            foreach ($cell in $cells) {
                $cellHtml = if ($LinkStructNames) {
                    ConvertTo-LinkedInterfaceHtmlText $cell
                } else {
                    ConvertTo-LinkedHtmlText $cell $false
                }
                [void]$body.AppendLine("      <$tag>$cellHtml</$tag>")
            }
            [void]$body.AppendLine('    </tr>')
            continue
        }
        if ($inTable) {
            [void]$body.AppendLine('  </table>')
            $inTable = $false
        }
        if ($line -match '^\-\s+(.+)$') {
            $lineHtml = if ($LinkStructNames) { ConvertTo-LinkedInterfaceHtmlText $Matches[1] } else { ConvertTo-LinkedHtmlText $Matches[1] $false }
            [void]$body.AppendLine("  <p>- $lineHtml</p>")
        } elseif ($line -match '^>\s*(.+)$') {
            $lineHtml = if ($LinkStructNames) { ConvertTo-LinkedInterfaceHtmlText $Matches[1] } else { ConvertTo-LinkedHtmlText $Matches[1] $false }
            [void]$body.AppendLine("  <p>$lineHtml</p>")
        } else {
            $lineHtml = if ($LinkStructNames) { ConvertTo-LinkedInterfaceHtmlText $line } else { ConvertTo-LinkedHtmlText $line $false }
            [void]$body.AppendLine("  <p>$lineHtml</p>")
        }
    }
    if ($inTable) {
        [void]$body.AppendLine('  </table>')
    }
    [void]$body.AppendLine('</body>')
    [void]$body.AppendLine('</html>')
    return $body.ToString()
}

function New-CallbackIndexHtml($Rows) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('| 类型 | 功能描述 | 接口 | 相关参数 |')
    $lines.Add('|---|---|---|---|')
    foreach ($row in $Rows) {
        $lines.Add(('| {0} | {1} | `{2}` | {3} |' -f $row.Type, $row.Description, $row.Interface, $row.ParamsClean))
    }
    return New-TopicHtml "接口回调" ($lines.ToArray()) $true
}

function New-CallbackDetailHtml($Row, $Prototypes) {
    $signature = ""
    if ($Prototypes.ContainsKey($Row.Interface)) {
        $signature = $Prototypes[$Row.Interface]
    } else {
        $paramText = $Row.RawParamText
        if ([string]::IsNullOrWhiteSpace($paramText)) {
            $paramText = "void"
        }
        $signature = "NET_TV_API BOOL STDCALL $($Row.Interface)($paramText);"
    }

    $signature = $signature -replace 'NET_TV_API\s+', ''
    $signature = $signature -replace 'STDCALL\s+', ''
    $signature = $signature.Trim()
    $paramsInside = ""
    if ($signature -match '\((.*)\)\s*;?\s*$') {
        $paramsInside = $Matches[1]
    }
    $paramDecls = @(Split-ParameterList $paramsInside)
    $relatedStructs = @(Get-StructNamesFromText $Row.ParamsClean)

    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('<!doctype html>')
    [void]$body.AppendLine('<html>')
    [void]$body.AppendLine('<head>')
    [void]$body.AppendLine('  <meta charset="utf-8">')
    [void]$body.AppendLine("  <title>$(ConvertTo-HtmlText $Row.Interface)</title>")
    [void]$body.AppendLine('  <style>')
    [void]$body.AppendLine('    body { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; color: #000; }')
    [void]$body.AppendLine('    h1 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 0 0 8px; }')
    [void]$body.AppendLine('    h2 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 18px 0 6px; }')
    [void]$body.AppendLine('    p { margin: 0 0 6px; }')
    [void]$body.AppendLine('    .code-block { background-color: #e6e6e6; border: 0; padding: 8px 10px; margin: 12px 0 14px; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$body.AppendLine('    .code-line { margin: 0; padding: 0; min-height: 1.45em; white-space: pre-wrap; background-color: #e6e6e6; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$body.AppendLine('    .code-line a { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$body.AppendLine('    .param-name { margin-left: 0; }')
    [void]$body.AppendLine('    .param-desc { margin-left: 24px; }')
    [void]$body.AppendLine('  </style>')
    [void]$body.AppendLine('</head>')
    [void]$body.AppendLine('<body>')
    [void]$body.AppendLine("  <h1>$(ConvertTo-HtmlText $Row.Interface)</h1>")
    [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $Row.Description)</p>")
    [void]$body.Append((ConvertTo-CodeBlockHtml $signature $true))

    [void]$body.AppendLine('  <h2>Parameters</h2>')
    if ($paramDecls.Count -eq 0) {
        [void]$body.AppendLine('  <p>无参数。</p>')
    } else {
        foreach ($paramDecl in $paramDecls) {
            $paramName = Get-ParamName $paramDecl
            $direction = Get-ParamDirection $paramDecl
            $desc = Get-ParamDescription $paramName $paramDecl $Row
            [void]$body.AppendLine("  <p class=""param-name"">[$direction] $(ConvertTo-HtmlText $paramName)</p>")
            [void]$body.AppendLine("  <p class=""param-desc"">$(ConvertTo-HtmlText $desc)</p>")
        }
    }

    [void]$body.AppendLine('  <h2>Return Values</h2>')
    [void]$body.AppendLine('  <p>BOOL 类型返回值。TRUE 表示成功，FALSE 表示失败。接口返回失败时可调用 NET_TV_GetLastError 获取错误码。</p>')

    [void]$body.AppendLine('  <h2>Remarks</h2>')
    [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $Row.Description)</p>")
    if (-not [string]::IsNullOrWhiteSpace($Row.CommandText)) {
        [void]$body.AppendLine("  <p>关联命令：$(ConvertTo-HtmlText $Row.CommandText)</p>")
    }

    [void]$body.AppendLine('  <h2>See Also</h2>')
    if ($relatedStructs.Count -gt 0) {
        foreach ($structName in $relatedStructs) {
            [void]$body.AppendLine("  <p><a href=""$structName.html"">$structName</a></p>")
        }
    } else {
        [void]$body.AppendLine('  <p>无。</p>')
    }

    [void]$body.AppendLine('</body>')
    [void]$body.AppendLine('</html>')
    return $body.ToString()
}

function Parse-CallbackRows([string[]]$Lines) {
    $rows = New-Object System.Collections.Generic.List[object]
    foreach ($line in $Lines) {
        if (-not $line.Trim().StartsWith('|') -or (Test-TableSeparator $line)) {
            continue
        }
        $cells = Split-MarkdownTableRow $line
        if ($cells.Count -lt 4 -or $cells[0] -eq '类型') {
            continue
        }

        $interfaceValues = @(Get-BacktickValues $cells[2])
        if ($interfaceValues.Count -eq 0) {
            continue
        }

        $paramsClean = Remove-StatusFromParamText $cells[3]
        $rawParamText = $paramsClean
        if ($rawParamText -match '参数\s*:\s*(.+)$') {
            $rawParamText = $Matches[1].Trim()
        }
        $commandText = ""
        if ($paramsClean -match '命令\s*:\s*([^;；]+)') {
            $commandText = ($Matches[1] -replace '`', '').Trim()
        }

        $rows.Add([pscustomobject]@{
            Type = CleanInline $cells[0]
            Description = CleanInline $cells[1]
            Interface = $interfaceValues[0]
            ParamsClean = $paramsClean
            RawParamText = $rawParamText
            CommandText = $commandText
        })
    }
    return $rows.ToArray()
}

function Get-FunctionPrototypes([string]$HeaderPath) {
    $result = @{}
    if (-not (Test-Path -LiteralPath $HeaderPath)) {
        return $result
    }

    $content = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $HeaderPath).Path, [System.Text.Encoding]::UTF8)
    foreach ($match in [regex]::Matches($content, 'NET_TV_API\s+[^;]+?\b(NET_TV[A-Za-z0-9_]+)\s*\([^;]*?\);', [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
        $name = $match.Groups[1].Value
        $signature = [regex]::Replace($match.Value, '\s+', ' ').Trim()
        $signature = $signature -replace '\(\s+', '('
        $signature = $signature -replace '\s+\)', ')'
        $signature = $signature -replace ',\s*', ', '
        $result[$name] = $signature
    }
    return $result
}

function Get-StructDefinitions([string]$HeaderPath) {
    $result = @{}
    if (-not (Test-Path -LiteralPath $HeaderPath)) {
        return $result
    }

    $content = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $HeaderPath).Path, [System.Text.Encoding]::UTF8)
    foreach ($match in [regex]::Matches($content, 'typedef\s+struct\s+[A-Za-z0-9_]*\s*\{.*?\}\s*(NET_TV_[A-Za-z0-9_]+_S)\s*,\s*\*?LP?NET_TV_[A-Za-z0-9_]+_S\s*;', [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
        $name = $match.Groups[1].Value
        $definition = $match.Value.Trim()
        $definition = [regex]::Replace($definition, '\r\n|\r|\n', "`n")
        $result[$name] = $definition
    }
    return $result
}

function Parse-StructTopicLines([string]$Title, [string[]]$Lines) {
    $description = ""
    $sourceLine = ""
    $alias = ""
    $fields = New-Object System.Collections.Generic.List[object]

    foreach ($line in $Lines) {
        if ($line -match '^\-\s+说明：(.+)$') {
            $description = CleanInline $Matches[1]
            continue
        }
        if ($line -match '^\-\s+源码行：(.+)$') {
            $sourceLine = CleanInline $Matches[1]
            continue
        }
        if ($line -match '^\-\s+别名：(.+)$') {
            $alias = CleanInline $Matches[1]
            continue
        }
        if ($line.Trim().StartsWith('|') -and -not (Test-TableSeparator $line)) {
            $cells = Split-MarkdownTableRow $line
            if ($cells.Count -ge 2 -and $cells[0] -ne '字段声明') {
                $decl = CleanInline $cells[0]
                $desc = CleanInline $cells[1]
                $fields.Add([pscustomobject]@{
                    Declaration = $decl
                    Name = Get-ParamName $decl
                    Direction = "in"
                    Description = $desc
                })
            }
        }
    }

    return [pscustomobject]@{
        Title = $Title
        Description = $description
        SourceLine = $sourceLine
        Alias = $alias
        Fields = $fields.ToArray()
    }
}

function New-StructDefinitionFromFields($StructInfo) {
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine('struct')
    [void]$sb.AppendLine('{')
    foreach ($field in $StructInfo.Fields) {
        [void]$sb.AppendLine("    $($field.Declaration);")
    }
    [void]$sb.AppendLine('};')
    return $sb.ToString().Trim()
}

function New-StructDetailHtml($StructInfo, [string]$Definition) {
    if ($StructInfo.Fields.Count -gt 0) {
        $Definition = New-StructDefinitionFromFields $StructInfo
    } elseif ([string]::IsNullOrWhiteSpace($Definition)) {
        $Definition = New-StructDefinitionFromFields $StructInfo
    }

    $body = New-Object System.Text.StringBuilder
    [void]$body.AppendLine('<!doctype html>')
    [void]$body.AppendLine('<html>')
    [void]$body.AppendLine('<head>')
    [void]$body.AppendLine('  <meta charset="utf-8">')
    [void]$body.AppendLine("  <title>$(ConvertTo-HtmlText $StructInfo.Title)</title>")
    [void]$body.AppendLine('  <style>')
    [void]$body.AppendLine('    body { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; color: #000; }')
    [void]$body.AppendLine('    h1 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 0 0 6px; }')
    [void]$body.AppendLine('    h2 { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; font-weight: bold; margin: 18px 0 6px; }')
    [void]$body.AppendLine('    p { margin: 0 0 6px; }')
    [void]$body.AppendLine('    .code-block { background-color: #e6e6e6; border: 0; padding: 8px 10px; margin: 14px 0 18px; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$body.AppendLine('    .code-line { margin: 0; padding: 0; min-height: 1.45em; white-space: pre-wrap; background-color: #e6e6e6; font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; line-height: 1.45; }')
    [void]$body.AppendLine('    .code-line a { font-family: "Microsoft YaHei", "微软雅黑", Arial, sans-serif; font-size: 10pt; }')
    [void]$body.AppendLine('    .param-name { margin-left: 0; }')
    [void]$body.AppendLine('    .param-desc { margin-left: 24px; }')
    [void]$body.AppendLine('  </style>')
    [void]$body.AppendLine('</head>')
    [void]$body.AppendLine('<body>')
    [void]$body.AppendLine("  <h1>$(ConvertTo-HtmlText $StructInfo.Title)</h1>")
    if (-not [string]::IsNullOrWhiteSpace($StructInfo.Description)) {
        [void]$body.AppendLine("  <p>$(ConvertTo-HtmlText $StructInfo.Description)</p>")
    }
    [void]$body.Append((ConvertTo-CodeBlockHtml $Definition $true))

    [void]$body.AppendLine('  <h2>Parameters</h2>')
    if ($StructInfo.Fields.Count -eq 0) {
        [void]$body.AppendLine('  <p>无字段。</p>')
    } else {
        foreach ($field in $StructInfo.Fields) {
            [void]$body.AppendLine("  <p class=""param-name"">[$($field.Direction)] $(ConvertTo-HtmlText $field.Name)</p>")
            [void]$body.AppendLine("  <p class=""param-desc"">$(ConvertTo-HtmlText $field.Description)</p>")
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

    [void]$body.AppendLine('</body>')
    [void]$body.AppendLine('</html>')
    return $body.ToString()
}

function Add-Topic($List, [string]$Section, [string]$Title, [string[]]$Lines) {
    $fileName = (ConvertTo-SafeFileName $Title) + ".html"
    $helpId = ConvertTo-HelpId $Title
    if ($Section -eq 'callback' -and $helpId -eq 'topic') {
        $helpId = 'Interface_Callback'
    }
    $List.Add([pscustomobject]@{
        Section = $Section
        Title = $Title
        HelpId = $helpId
        FileName = $fileName
        Lines = $Lines
    })
}

function Add-HtmlTopic($List, [string]$Section, [string]$Title, [string]$ParentSection, [string]$Html) {
    $fileName = (ConvertTo-SafeFileName $Title) + ".html"
    $helpId = ConvertTo-HelpId $Title
    if ($Section -eq 'callback_index') {
        $helpId = 'Interface_Callback'
    }
    $List.Add([pscustomobject]@{
        Section = $Section
        ParentSection = $ParentSection
        Title = $Title
        HelpId = $helpId
        FileName = $fileName
        Lines = @()
        Html = $Html
    })
}

function Write-TopicFile($Topic, [string]$Dir) {
    $path = Join-Path $Dir $Topic.FileName
    if ($Topic.PSObject.Properties.Name -contains 'Html' -and -not [string]::IsNullOrWhiteSpace($Topic.Html)) {
        $html = $Topic.Html
    } else {
        $html = New-TopicHtml $Topic.Title $Topic.Lines ($Topic.Section -eq 'callback')
    }
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($path, $html, $encoding)
}

function Escape-Pascal([string]$Text) {
    return $Text -replace "'", "''"
}

function New-ImportScript($Topics, [string]$OutputDir, [string]$RootTitle, [string]$StructTitle, [bool]$HasCallbackTopic) {
    $fullDir = (Resolve-Path -LiteralPath $OutputDir).Path
    $structTopics = @($Topics | Where-Object { $_.Section -eq 'struct' })
    $callbackIndexTopics = @($Topics | Where-Object { $_.Section -eq 'callback_index' })
    $callbackDetailTopics = @($Topics | Where-Object { $_.Section -eq 'callback_detail' })

    $script = New-Object System.Text.StringBuilder
    [void]$script.AppendLine('// HelpNDoc Pascal Script')
    [void]$script.AppendLine('// 在 HelpNDoc 中打开目标 .hnd 工程后，运行本脚本。')
    if ($HasCallbackTopic) {
        [void]$script.AppendLine('// 作用：自动创建接口回调、接口明细和接口参数结构体 topic，并导入对应 HTML 内容。')
    } else {
        [void]$script.AppendLine('// 作用：按接口参数结构体自动创建 topic，并导入对应 HTML 内容。')
    }
    [void]$script.AppendLine('')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  RootTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  StructTopic: string;')
    if ($HasCallbackTopic) {
        [void]$script.AppendLine('var')
        [void]$script.AppendLine('  CallbackTopic: string;')
    }
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  NewTopic: string;')
    [void]$script.AppendLine('var')
    [void]$script.AppendLine('  Editor: TObject;')
    [void]$script.AppendLine('')
    [void]$script.AppendLine('begin')
    [void]$script.AppendLine('  Editor := HndEditor.CreateTemporaryEditor();')
    [void]$script.AppendLine('  RootTopic := HndTopics.CreateTopic();')
    [void]$script.AppendLine("  HndTopics.SetTopicCaption(RootTopic, '$(Escape-Pascal $RootTitle)');")
    [void]$script.AppendLine("  HndTopics.SetTopicHelpId(RootTopic, '$(Escape-Pascal (ConvertTo-HelpId $RootTitle))');")
    [void]$script.AppendLine('')

    if ($HasCallbackTopic) {
        if ($callbackIndexTopics.Count -gt 0) {
            $callback = $callbackIndexTopics[0]
            $htmlPath = Join-Path $fullDir $callback.FileName
            [void]$script.AppendLine('  CallbackTopic := HndTopics.CreateTopic();')
            [void]$script.AppendLine("  HndTopics.SetTopicCaption(CallbackTopic, '$(Escape-Pascal $callback.Title)');")
            [void]$script.AppendLine("  HndTopics.SetTopicHelpId(CallbackTopic, '$(Escape-Pascal $callback.HelpId)');")
            [void]$script.AppendLine('  HndTopics.MoveTopic(CallbackTopic, RootTopic, htamAddChild);')
            [void]$script.AppendLine('  HndEditor.Clear(Editor);')
            [void]$script.AppendLine("  HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
            [void]$script.AppendLine('  HndEditor.SetAsTopicContent(Editor, CallbackTopic);')
            [void]$script.AppendLine('')
        }

        foreach ($topic in $callbackDetailTopics) {
            $htmlPath = Join-Path $fullDir $topic.FileName
            [void]$script.AppendLine("  NewTopic := HndTopics.CreateTopic();")
            [void]$script.AppendLine("  HndTopics.SetTopicCaption(NewTopic, '$(Escape-Pascal $topic.Title)');")
            [void]$script.AppendLine("  HndTopics.SetTopicHelpId(NewTopic, '$(Escape-Pascal $topic.HelpId)');")
            [void]$script.AppendLine("  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);")
            [void]$script.AppendLine('  HndEditor.Clear(Editor);')
            [void]$script.AppendLine("  HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
            [void]$script.AppendLine('  HndEditor.SetAsTopicContent(Editor, NewTopic);')
            [void]$script.AppendLine('')
        }
    }

    [void]$script.AppendLine('  StructTopic := HndTopics.CreateTopic();')
    [void]$script.AppendLine("  HndTopics.SetTopicCaption(StructTopic, '$(Escape-Pascal $StructTitle)');")
    [void]$script.AppendLine('  HndTopics.MoveTopic(StructTopic, RootTopic, htamAddChild);')
    [void]$script.AppendLine('')
    foreach ($topic in $structTopics) {
        $htmlPath = Join-Path $fullDir $topic.FileName
        [void]$script.AppendLine("  NewTopic := HndTopics.CreateTopic();")
        [void]$script.AppendLine("  HndTopics.SetTopicCaption(NewTopic, '$(Escape-Pascal $topic.Title)');")
        [void]$script.AppendLine("  HndTopics.SetTopicHelpId(NewTopic, '$(Escape-Pascal $topic.HelpId)');")
        [void]$script.AppendLine("  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);")
        [void]$script.AppendLine('  HndEditor.Clear(Editor);')
        [void]$script.AppendLine("  HndEditor.InsertFile(Editor, '$(Escape-Pascal $htmlPath)');")
        [void]$script.AppendLine('  HndEditor.SetAsTopicContent(Editor, NewTopic);')
        [void]$script.AppendLine('')
    }

    [void]$script.AppendLine('  HndEditor.DestroyTemporaryEditor(Editor);')
    [void]$script.AppendLine('end.')
    return $script.ToString()
}

$markdownFullPath = (Resolve-Path -LiteralPath $MarkdownPath).Path
$outputFullPath = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDir))
if (Test-Path -LiteralPath $outputFullPath) {
    Remove-Item -LiteralPath $outputFullPath -Recurse -Force
}
New-Item -ItemType Directory -Path $outputFullPath -Force | Out-Null

$lines = [System.IO.File]::ReadAllLines($markdownFullPath, [System.Text.Encoding]::UTF8)
$topics = New-Object System.Collections.Generic.List[object]
$callbackRowsForTopics = @()
$structInfosForTopics = @()

$section = ''
$tableHeader = $null
for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    if ($IncludeCallbackTopic -and $line -match '^##\s+3\.') {
        $sectionLines = New-Object System.Collections.Generic.List[string]
        $j = $i + 1
        while ($j -lt $lines.Count -and $lines[$j] -notmatch '^##\s+') {
            if (-not [string]::IsNullOrWhiteSpace($lines[$j])) {
                $sectionLines.Add($lines[$j])
            }
            $j++
        }
        $callbackRowsForTopics = @(Parse-CallbackRows ($sectionLines.ToArray()))
        foreach ($row in $callbackRowsForTopics) {
            $script:CallbackInterfaceNameSet[$row.Interface] = $true
        }
        $i = $j - 1
        continue
    }

    if ($line -match '^##\s+6\.') {
        $section = 'struct'
        continue
    }
    if ($line -match '^##\s+7\.') {
        $section = ''
        continue
    }

    if ($section -eq 'struct' -and $line -match '^###\s+(NET_TV[A-Za-z0-9_]+)') {
        $title = $Matches[1]
        $topicLines = New-Object System.Collections.Generic.List[string]
        $topicLines.Add($line)
        $j = $i + 1
        while ($j -lt $lines.Count -and $lines[$j] -notmatch '^###\s+NET_TV' -and $lines[$j] -notmatch '^##\s+') {
            $topicLines.Add($lines[$j])
            $j++
        }
        $structInfosForTopics += Parse-StructTopicLines $title ($topicLines.ToArray())
        $i = $j - 1
    }
}

$script:StructTopicNameSet = @{}
foreach ($structInfo in $structInfosForTopics) {
    $script:StructTopicNameSet[$structInfo.Title] = $true
}

$structDefinitions = Get-StructDefinitions $CommonHeaderPath
foreach ($structInfo in @($structInfosForTopics | Sort-Object Title)) {
    $definition = ""
    if ($structDefinitions.ContainsKey($structInfo.Title)) {
        $definition = $structDefinitions[$structInfo.Title]
    }
    Add-HtmlTopic $topics 'struct' $structInfo.Title '' (New-StructDetailHtml $structInfo $definition)
}

if ($IncludeCallbackTopic -and $callbackRowsForTopics.Count -gt 0) {
    Add-HtmlTopic $topics 'callback_index' $CallbackTopicTitle '' (New-CallbackIndexHtml $callbackRowsForTopics)
    $prototypes = Get-FunctionPrototypes $InterfaceHeaderPath
    foreach ($row in @($callbackRowsForTopics | Sort-Object Interface)) {
        Add-HtmlTopic $topics 'callback_detail' $row.Interface 'callback_index' (New-CallbackDetailHtml $row $prototypes)
    }
}

$topics = @($topics | Sort-Object @{ Expression = {
    switch ($_.Section) {
        'callback_index' { 0 }
        'callback_detail' { 1 }
        'struct' { 2 }
        default { 9 }
    }
}}, Title)

foreach ($topic in $topics) {
    Write-TopicFile $topic $outputFullPath
}

$hasCallbackTopic = @($topics | Where-Object { $_.Section -eq 'callback_index' }).Count -gt 0
$importScript = New-ImportScript $topics $outputFullPath $RootTopicTitle $StructTopicTitle $hasCallbackTopic
$scriptPath = Join-Path $outputFullPath "import_to_helpndoc.pas"
$encoding = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($scriptPath, $importScript, $encoding)

$manifestPath = Join-Path $outputFullPath "topics_manifest.csv"
$topics |
    Select-Object Section, ParentSection, Title, HelpId, FileName |
    Export-Csv -LiteralPath $manifestPath -Encoding UTF8 -NoTypeInformation

Write-Host "Output directory: $outputFullPath"
Write-Host "Topic files: $($topics.Count)"
Write-Host "HelpNDoc import script: $scriptPath"
Write-Host "Manifest: $manifestPath"
