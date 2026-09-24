# Config/Tags/Native 아래의 태그 ini들로부터 KataTag.A.B 형태로 접근하는 네이티브 게임플레이 태그 코드를 생성한다.
# ProjectKata.uproject의 PreBuildSteps가 매 빌드마다 호출한다. 계획과 결정 근거는 docs/plan/Gameplay-Tag-Plan.md를 따른다.
#
# 생성 결과
# - 헤더: <RootName>Private 네임스페이스의 FNativeGameplayTag extern 선언과 계층 구조체, inline 루트 객체.
# - 소스: UE_DEFINE_GAMEPLAY_TAG_COMMENT 정의. 엔진 매크로가 .cpp에서만 허용되므로 정의는 반드시 소스에 둔다.
#
# 이 스크립트는 Windows PowerShell 5.1에서도 실행되어야 하므로 UTF-8 BOM을 붙여 저장한다.
# BOM이 없으면 5.1이 시스템 코드 페이지로 읽어 한국어 주석이 깨진다.
[CmdletBinding()]
param(
    [string]$TagsDir,
    [string]$NativeDir,
    [string]$OutputHeader,
    [string]$OutputSource,
    [string]$RootName = 'KataTag',
    [string]$ApiMacro = 'PROJECTKATA_API'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

# 경로를 인수로 받지 않으면 이 스크립트가 있는 프로젝트의 기본 위치를 사용한다.
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($TagsDir)) { $TagsDir = Join-Path $projectRoot 'Config\Tags' }
if ([string]::IsNullOrWhiteSpace($NativeDir)) { $NativeDir = Join-Path $TagsDir 'Native' }
if ([string]::IsNullOrWhiteSpace($OutputHeader)) { $OutputHeader = Join-Path $projectRoot "Source\ProjectKata\${RootName}s.h" }
if ([string]::IsNullOrWhiteSpace($OutputSource)) { $OutputSource = Join-Path $projectRoot "Source\ProjectKata\${RootName}s.cpp" }

$logPrefix = "[${RootName}s]"
$errors = New-Object System.Collections.Generic.List[string]

function Add-GenError([string]$File, [int]$Line, [string]$Message) {
    # MSVC 형식으로 출력해 IDE 출력 창에서 위치로 이동할 수 있게 한다.
    if ([string]::IsNullOrEmpty($File)) {
        $script:errors.Add("error: $Message")
    } elseif ($Line -gt 0) {
        $script:errors.Add("${File}(${Line}): error: $Message")
    } else {
        $script:errors.Add("${File}: error: $Message")
    }
}

# C++ 키워드와 대체 토큰, 그리고 엔진·Windows 헤더가 매크로로 정의해 멤버 이름으로 쓰면 치환되는 이름.
$reservedNames = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
@(
    'alignas', 'alignof', 'and', 'and_eq', 'asm', 'auto', 'bitand', 'bitor', 'bool', 'break', 'case', 'catch',
    'char', 'char8_t', 'char16_t', 'char32_t', 'class', 'compl', 'concept', 'const', 'consteval', 'constexpr',
    'constinit', 'const_cast', 'continue', 'co_await', 'co_return', 'co_yield', 'decltype', 'default', 'delete',
    'do', 'double', 'dynamic_cast', 'else', 'enum', 'explicit', 'export', 'extern', 'false', 'float', 'for',
    'friend', 'goto', 'if', 'inline', 'int', 'long', 'mutable', 'namespace', 'new', 'noexcept', 'not', 'not_eq',
    'nullptr', 'operator', 'or', 'or_eq', 'private', 'protected', 'public', 'register', 'reinterpret_cast',
    'requires', 'return', 'short', 'signed', 'sizeof', 'static', 'static_assert', 'static_cast', 'struct',
    'switch', 'template', 'this', 'thread_local', 'throw', 'true', 'try', 'typedef', 'typeid', 'typename',
    'union', 'unsigned', 'using', 'virtual', 'void', 'volatile', 'wchar_t', 'while', 'xor', 'xor_eq',
    'TEXT', 'check', 'checkf', 'checkSlow', 'checkNoEntry', 'checkNoReentry', 'checkNoRecursion', 'verify',
    'verifyf', 'ensure', 'ensureMsgf', 'ensureAlways', 'ensureAlwaysMsgf', 'UE_LOG', 'TRUE', 'FALSE', 'NULL',
    'IN', 'OUT', 'OPTIONAL', 'CONST', 'VOID', 'DELETE', 'ERROR', 'IGNORE', 'INFINITE', 'UNICODE', 'PI'
) | ForEach-Object { [void]$reservedNames.Add($_) }

function Test-TagSegment([string]$Segment) {
    # 첫 글자를 영문자로 제한해 밑줄로 시작하는 예약 식별자를 피한다.
    # 조각을 밑줄로 이어 변수 이름을 만들므로 연속 밑줄(예약 식별자)이 생기는 형태도 막는다.
    if ($Segment -cnotmatch '^[A-Za-z][A-Za-z0-9_]*$') { return 'must start with a letter and contain only letters, digits, and underscores' }
    if ($Segment.EndsWith('_') -or $Segment.Contains('__')) { return 'must not end with an underscore or contain consecutive underscores' }
    if ($reservedNames.Contains($Segment)) { return 'is a C++ keyword or an engine/Windows macro name' }
    return $null
}

function Read-QuotedValue([string]$Body, [string]$Key) {
    $match = [regex]::Match($Body, '(?:^|,)\s*' + $Key + '\s*=\s*(?:"((?:[^"\\]|\\.)*)"|([^,]*))')
    if (-not $match.Success) { return $null }
    if ($match.Groups[1].Success) { return [regex]::Replace($match.Groups[1].Value, '\\(.)', '$1') }
    return $match.Groups[2].Value.Trim()
}

# 1. Config/Tags 전체의 ini 파일 이름 충돌 검사.
# 엔진은 Config/Tags를 재귀로 읽지만 태그 소스를 파일 이름만으로 구분한다. 이름이 같으면 폴더가 달라도 한 소스로 합쳐진다.
if (Test-Path -LiteralPath $TagsDir -PathType Container) {
    Get-ChildItem -LiteralPath $TagsDir -Filter '*.ini' -Recurse -File |
        Group-Object -Property Name |
        Where-Object { $_.Count -gt 1 } |
        ForEach-Object {
            $paths = ($_.Group | ForEach-Object { $_.FullName }) -join ', '
            Add-GenError $_.Group[0].FullName 0 "Tag ini file name '$($_.Name)' is used more than once under Config/Tags. The engine merges tag sources by file name only: $paths"
        }
}

# 2. Native ini 파싱.
# 태그는 FName이므로 대소문자를 구분하지 않는다. 대소문자만 다른 태그는 같은 태그로 보고 중복으로 처리한다.
$declaredTags = New-Object 'System.Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)
$nativeFiles = @()
if (Test-Path -LiteralPath $NativeDir -PathType Container) {
    $nativeFiles = @(Get-ChildItem -LiteralPath $NativeDir -Filter '*.ini' -Recurse -File | Sort-Object -Property FullName)
}

foreach ($file in $nativeFiles) {
    # 엔진은 한국어 등 비ASCII 문자가 들어간 설정 파일을 UTF-16으로 저장할 수 있다. ReadAllText가 BOM으로 인코딩을 판별한다.
    $lines = [System.IO.File]::ReadAllText($file.FullName) -split "`r?`n"
    $inTagSection = $false
    for ($index = 0; $index -lt $lines.Length; $index++) {
        $line = $lines[$index]
        $lineNumber = $index + 1

        $sectionMatch = [regex]::Match($line, '^\s*\[(.+)\]\s*$')
        if ($sectionMatch.Success) {
            $inTagSection = ($sectionMatch.Groups[1].Value.Trim() -eq '/Script/GameplayTags.GameplayTagsList')
            continue
        }
        if (-not $inTagSection) { continue }

        $entryMatch = [regex]::Match($line, '^\s*([+\-.!@*^]?)GameplayTagList\s*=\s*\((.*)\)\s*$')
        if (-not $entryMatch.Success) { continue }

        if ($entryMatch.Groups[1].Value -ne '') {
            # 엔진은 Config/Tags의 개별 ini를 FConfigFile::Read로 읽고, 이 경로는 +·- 같은 ini 명령 기호를 처리하지 않는다.
            # 기호가 붙은 줄은 키 이름이 달라져 엔진이 무시하므로 태그 소스 목록에서 빠진다. 에디터가 이 파일에 태그를 추가하면
            # 소스 목록만 다시 기록하므로 해당 줄이 지워진다. 생성 코드와 엔진의 해석이 어긋나지 않게 빌드를 실패시킨다.
            Add-GenError $file.FullName $lineNumber "Remove the '$($entryMatch.Groups[1].Value)' prefix. Tag ini files under Config/Tags are read without config commands, so the engine ignores this line. Use 'GameplayTagList=(...)'."
            continue
        }

        $body = $entryMatch.Groups[2].Value
        $tagName = Read-QuotedValue $body 'Tag'
        if ([string]::IsNullOrWhiteSpace($tagName)) {
            Add-GenError $file.FullName $lineNumber 'GameplayTagList entry has no Tag value.'
            continue
        }
        $tagName = $tagName.Trim()
        $comment = Read-QuotedValue $body 'DevComment'
        if ($null -eq $comment) { $comment = '' }

        $segments = $tagName.Split('.')
        $invalid = $false
        foreach ($segment in $segments) {
            if ($segment.Length -eq 0) {
                Add-GenError $file.FullName $lineNumber "Tag '$tagName' has an empty segment."
                $invalid = $true
                break
            }
            $reason = Test-TagSegment $segment
            if ($null -ne $reason) {
                Add-GenError $file.FullName $lineNumber "Tag '$tagName' segment '$segment' $reason."
                $invalid = $true
                break
            }
        }
        if ($invalid) { continue }

        if ($declaredTags.ContainsKey($tagName)) {
            $previous = $declaredTags[$tagName]
            Add-GenError $file.FullName $lineNumber "Tag '$tagName' is already declared at $($previous.File)($($previous.Line))."
            continue
        }

        $declaredTags[$tagName] = [pscustomobject]@{
            Name = $tagName
            Segments = $segments
            Comment = $comment
            File = $file.FullName
            Line = $lineNumber
        }
    }
}

# 3. 트리 구성.
# 선언하지 않은 중간 부모도 노드로 만든다. 부모 노드를 태그로 변환하려면 그 태그의 네이티브 정의가 필요하기 때문이다.
$rootNode = [pscustomobject]@{ FullName = ''; Segment = ''; Path = @(); Comment = ''; Children = (New-Object 'System.Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)) }
$allNodes = New-Object System.Collections.Generic.List[object]

foreach ($declared in $declaredTags.Values) {
    $current = $rootNode
    $path = @()
    for ($depth = 0; $depth -lt $declared.Segments.Length; $depth++) {
        $segment = $declared.Segments[$depth]
        if ($current.Children.ContainsKey($segment)) {
            $current = $current.Children[$segment]
            $path = $current.Path
        } else {
            $path = @($path + $segment)
            $child = [pscustomobject]@{
                FullName = ($path -join '.')
                Segment = $segment
                Path = $path
                Comment = ''
                Children = (New-Object 'System.Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase))
            }
            $current.Children[$segment] = $child
            $allNodes.Add($child)
            $current = $child
        }
    }
    $current.Comment = $declared.Comment
}

# 조각을 밑줄로 이어 만든 변수 이름은 A_B.C와 A.B_C처럼 서로 다른 태그에서 같아질 수 있다.
$flatNames = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
foreach ($node in $allNodes) {
    $flat = $node.Path -join '_'
    if ($flatNames.ContainsKey($flat)) {
        Add-GenError '' 0 "Tags '$($flatNames[$flat])' and '$($node.FullName)' both map to the C++ name '$flat'. Rename one of them."
    } else {
        $flatNames[$flat] = $node.FullName
    }
}

if ($errors.Count -gt 0) {
    foreach ($message in $errors) { [Console]::Error.WriteLine($message) }
    [Console]::Error.WriteLine("$logPrefix Native gameplay tag generation failed with $($errors.Count) error(s).")
    exit 1
}

# 4. 코드 생성.
$privateNamespace = "${RootName}Private"
$structPrefix = "F${RootName}_"
$rootStruct = "F${RootName}Root"
$headerName = Split-Path -Leaf $OutputHeader

function Get-SortedChildren($Node) {
    return @($Node.Children.Values | Sort-Object -Property Segment)
}

function Format-DocComment([string]$Comment) {
    # 주석 종료 기호와 줄바꿈이 생성 코드를 깨뜨리지 않게 정리한다.
    return (($Comment -replace '\*/', '* /') -replace "[`r`n]+", ' ').Trim()
}

function Format-CppString([string]$Value) {
    return (($Value -replace '\\', '\\') -replace '"', '\"') -replace "[`r`n]+", ' '
}

$generatedNotice = @(
    "// 이 파일은 Scripts/Generate-NativeGameplayTags.ps1이 Config/Tags/Native의 ini로부터 생성한다.",
    "// 직접 수정하지 않는다. 태그는 ini에 추가하거나 Project Settings에서 Native 폴더의 ini를 소스로 골라 추가한 뒤 다시 빌드한다."
)

$sortedNodes = @($allNodes | Sort-Object -Property FullName)

$header = New-Object System.Text.StringBuilder
foreach ($noticeLine in $generatedNotice) { [void]$header.Append($noticeLine).Append("`n") }
[void]$header.Append("`n#pragma once`n`n#include `"NativeGameplayTags.h`"`n`n")
[void]$header.Append("namespace $privateNamespace`n{`n")
foreach ($node in $sortedNodes) {
    [void]$header.Append("    extern $ApiMacro FNativeGameplayTag $($node.Path -join '_');`n")
}
[void]$header.Append("}`n")

# 자식 구조체를 부모보다 먼저 정의해야 하므로 후위 순회로 출력한다.
function Write-NodeStruct($Node, [System.Text.StringBuilder]$Builder) {
    $children = @(Get-SortedChildren $Node)
    foreach ($child in $children) {
        if ($child.Children.Count -gt 0) { Write-NodeStruct $child $Builder }
    }

    $isRoot = ($Node.Path.Length -eq 0)
    $structName = if ($isRoot) { $rootStruct } else { $structPrefix + ($Node.Path -join '_') }
    [void]$Builder.Append("`n")
    if ($isRoot) {
        [void]$Builder.Append("/** Config/Tags/Native에 정의한 태그 계층의 루트. $RootName 객체로 접근한다. */`n")
    } else {
        [void]$Builder.Append("/** $($Node.FullName) 태그와 하위 태그. 이 객체 자체는 $($Node.FullName) 태그로 변환된다. */`n")
    }
    [void]$Builder.Append("struct $structName`n{`n")
    foreach ($child in $children) {
        $doc = Format-DocComment $child.Comment
        if ($doc.Length -gt 0) { [void]$Builder.Append("    /** $doc */`n") }
        $variable = "${privateNamespace}::$($child.Path -join '_')"
        if ($child.Children.Count -gt 0) {
            [void]$Builder.Append("    $structPrefix$($child.Path -join '_') $($child.Segment);`n")
        } else {
            [void]$Builder.Append("    const FNativeGameplayTag& $($child.Segment) = $variable;`n")
        }
    }
    if (-not $isRoot) {
        if ($children.Count -gt 0) { [void]$Builder.Append("`n") }
        # FNativeGameplayTag의 변환 연산자가 값을 반환하므로 참조로 반환하면 임시 객체를 가리키게 된다.
        [void]$Builder.Append("    operator FGameplayTag() const { return ${privateNamespace}::$($Node.Path -join '_'); }`n")
    }
    [void]$Builder.Append("};`n")
}

Write-NodeStruct $rootNode $header
[void]$header.Append("`n/** 사용 예: Tag.MatchesTag($RootName.A) 또는 Container.HasTag($RootName.A.B). */`n")
[void]$header.Append("inline const $rootStruct $RootName{};`n")

$source = New-Object System.Text.StringBuilder
foreach ($noticeLine in $generatedNotice) { [void]$source.Append($noticeLine).Append("`n") }
[void]$source.Append("`n#include `"$headerName`"`n`n")
[void]$source.Append("namespace $privateNamespace`n{`n")
foreach ($node in $sortedNodes) {
    [void]$source.Append("    UE_DEFINE_GAMEPLAY_TAG_COMMENT($($node.Path -join '_'), `"$(Format-CppString $node.FullName)`", `"$(Format-CppString $node.Comment)`");`n")
}
[void]$source.Append("}`n")

function Write-IfChanged([string]$Path, [string]$Content) {
    # 내용이 같으면 쓰지 않는다. 헤더의 수정 시각이 바뀌면 이를 포함한 파일이 모두 다시 컴파일된다.
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        if ([System.IO.File]::ReadAllText($Path) -ceq $Content) { return $false }
    }
    $directory = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
        New-Item -ItemType Directory -Path $directory | Out-Null
    }
    [System.IO.File]::WriteAllText($Path, $Content, (New-Object System.Text.UTF8Encoding($false)))
    return $true
}

$headerWritten = Write-IfChanged $OutputHeader $header.ToString()
$sourceWritten = Write-IfChanged $OutputSource $source.ToString()

$state = if ($headerWritten -or $sourceWritten) { 'updated' } else { 'unchanged' }
Write-Output "$logPrefix $($declaredTags.Count) tag(s) from $($nativeFiles.Count) file(s), $($allNodes.Count) node(s): $state."
exit 0
