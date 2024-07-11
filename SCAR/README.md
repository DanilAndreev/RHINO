# Shader Compiler-Archiver (SCAR)

# Archive Structure

| PAYLOAD            | SIZE | OFFSET |
|--------------------|:----:|:------:|
| SCAR_VERSION_MAJOR |  1b  |   0b   |
| SCAR_VERSION_MINOR |  1b  |   1b   |
| SCAR_VERSION_PATCH |  1b  |   2b   |
| MAGIC_NUMBER       |  4b  |   4b   |


# Records Structure
## RootSignature
TODO: overview

| NAME | TYPE | COUNT | DESCRIPTION |
|-----:|:----:|:-----:|:------------|

## ShadersEntrypoints
|          NAME |    TYPE    | COUNT | DESCRIPTION                                             |
|--------------:|:----------:|:-----:|:--------------------------------------------------------|
| __EntryName__ | ```char``` |   x   | String entrypoints names separated with NUL characters. |

## RTAttributes
|                        NAME |    TYPE    | COUNT | DESCRIPTION                                                                                                                    |
|----------------------------:|:----------:|:-----:|:-------------------------------------------------------------------------------------------------------------------------------|
|   __maxPayloadSizeInBytes__ | ```char``` |   x   | The maximum storage for scalars (counted as 4 bytes each) in ray payloads in raytracing pipelines that contain this program.   |
| __maxAttributeSizeInBytes__ | ```char``` |   x   | The maximum number of scalars (counted as 4 bytes each) that can be used for attributes in pipelines that contain this shader. |
