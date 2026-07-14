# **Extended Functions**

## JSON Configuration File Description

Write the JSON file for operator definition. For details about the parameters, see [Table 1 Parameters in the JSON file](#Parameters_in_the_JSON_file) and [Table 2 test_cases_parameters](#test_cases_parameters).

For example, the JSON configuration file is named **add\_test.json**. Developers can modify test data and other configuration parameters based on this template.

```json
{
  "kernel_name": "add_custom",
  "kernel_path": "./add_custom.o",
  "blockdim": 8,
  "mode": "ca",
  "device_id": 0,
  "magic": "RT_DEV_BINARY_MAGIC_ELF_AIVEC",
  "test_cases": [
    {
      "case_name": "Test_AddCustom_001",
      "param_desc": [
        {
          "param_type": "input",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "data_path": "./input_x.bin",
          "name": "x"
        },
        {
          "param_type": "input",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "data_path": "./input_y.bin",
          "name": "y"
        },
        {
          "param_type": "output",
          "type": "float16",
          "shape": [
            8,
            2048
          ],
          "name": "z"
        },
        {
          "param_type": "workspace",
          "user_workspace_size": 4096
        },
        {
          "param_type": "tiling",
          "tiling_data_size": 8,
          "tiling_data_path": "./tiling.bin"
        }
      ]
    }
  ]
}
```

**Table 1** Parameters in the JSON file<a id="Parameters_in_the_JSON_file"></a>

|Parameter|Description|Type|Mandatory|
|---|---|---|---|
|kernel_name|Kernel function name.|string|Yes|
|kernel_path|Path of the binary .o file of the kernel function. The path can be either absolute or relative.|string|Yes|
|blockdim|Number of cores required for running the kernel function. The default value is **1**.|int|No|
|mode|Test mode.<br> - Onboard: **onboard**<br> - Performance simulation: **ca**|string|Yes|
|device_id|ID of the AI processor used for running. The default value is **0**.|int|No|
|tiling_key|Tiling key of the current dynamic operator.|uint64|No|
|magic|Operator type.<br> - Cube operator: **RT_DEV_BINARY_MAGIC_ELF_AICUBE**<br> - Vector operator: **RT_DEV_BINARY_MAGIC_ELF_AIVEC**<br> - Mixed fusion operator: **RT_DEV_BINARY_MAGIC_ELF** (only for <term>Atlas A3 training products, Atlas A3 inference products</term>, <term>Atlas A2 training products, and Atlas A2 inference products</term>)|string|Yes|
|test_cases|Test data. This can be a list, with each element containing a test case. For details, see [**Table 2** test_cases parameters](#test_cases_parameters).|list|Yes|

> [!NOTE]NOTE
> 
> - The **tiling\_key** parameter applies only to dynamic operators.
> - For <term>Atlas inference products</term>, the **magic** parameter must be set to **RT\_DEV\_BINARY\_MAGIC\_ELF**.
> - For operator on-board or simulation tuning, only one case can be configured for the **test\_cases** parameter.

Table 2 test\_cases parameters<a id="test_cases_parameters"></a>

<table><thead align="left"><tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row1958882429"><th class="cellrowborder" colspan="3" valign="top" id="mcps1.2.7.1.1"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1859148144216">Parameter</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.2"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p17594894219">Note</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.3"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p183566631519">Type</p>
</th>
<th class="cellrowborder" valign="top" id="mcps1.2.7.1.4"><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p766081315143">Mandatory</p>
</th>
</tr>
</thead>
<tbody><tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row7593813426"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p420842165820">case_name</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1620814217584">-</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p621330155713">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p620812225819">Test case name, which must be unique.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1356126191512">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p12660213201412">Yes</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row228167105811"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p161591620135916">param_desc</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p3159520185913">-</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p770893013570">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p4159162075916">Test case description. This can be a list, with each element representing a kernel function parameter.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p335610614152">list</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8625748205216">Yes</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row395535810590"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p7340135810572">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1395665819595">param_type</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1277910895615">input/output/workspace/tiling/fftsAddr</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p209523339431">Parameter type.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p113566611518">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p77111049195217">Yes</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row14519172175812"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16567163510586">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1751912215589">type</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p3526536165810">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p6519221125814">Supported input and output data types, such as **uint8**, **int16**, **int32**, **float16**, **float32** and **float**.</p>
<p id="p157634518485">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue139101824171913">**param_type**</span> is set to **input** or **output**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p153569610156">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p866017132146">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row16288640125810"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p10773183215215">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p12881405585">shape</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p15823211044">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16288040175819">Shapes supported by the input and output tensors. All input and output tensors must support the same number of shapes.</p>
<p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_zh-cn_topic_0000001214862872_p5132810134916">For example, **[8, 3, 256, 256]**.</p>
<p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_zh-cn_topic_0000001214862872_p1213211102498">If an invalid shape is entered, for example, **[0]**, an error is reported.</p>
<p id="p94333064915">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue10294141615195">**param_type**</span> is set to **input** or **output**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p10356176101520">list</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p76603135141">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row52426509594"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p17778832326">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p5242050165910">data_path</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p6828121843">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p324215055915">Path of the input data .bin file.</p>
<ul id="ul1083781334916"><li>When <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue47675439188">**param_type**</span> is set to **input**, **data_path** or **value_range** must be set, and **data_path** has a higher priority. </li><li>To set an empty **data_path** in the JSON file, set **"data_path":"null"**. For details about the JSON file, see <a href="#json-configuration-file-description">JSON Configuration Description</a>.</li></ul>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p135676171515">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p666081310140">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row1825765355911"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8783832225">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14258053165914">name</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p13833911544">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p7258175335917">Parameter name, which must be unique.</p>
<p id="p11342112324910">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue1010223718184">**param_type**</span> is set to **input** or **output**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p935666141513">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p16601513151410">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row13953193217319"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p07531721646">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p18953732531">user_workspace_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p77378388413">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p543694745013">Size of **workspace** set by the user.</p>
<p id="p196121931154911">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue679093317186">**param_type**</span> is set to **workspace**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p235676141519">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14660111371415">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row184573117510"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p156912346511">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p445711111156">tiling_data_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p121521036651">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p137712027185116">Size of **tiling** data.</p>
<p id="p151211383492">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue13366929171818">**param_type**</span> is set to **tiling**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p8357136191511">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p14660151381412">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row831318201955"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p13574734354">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p731318202052">tiling_data_path</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p715717361052">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1145144815519">Path of the tiling data .bin file.</p>
<p id="p6227174694915">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue18407152416188">**param_type**</span> is set to **tiling**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p735716681510">string</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1966018134148">No</p>
</td>
</tr>
<tr id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_row354884215210"><td class="cellrowborder" valign="top" width="9.49094909490949%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p618845215212">-</p>
</td>
<td class="cellrowborder" valign="top" width="14.411441144114413%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1718865275218">data_size</p>
</td>
<td class="cellrowborder" valign="top" width="22.832283228322833%" headers="mcps1.2.7.1.1 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p9188155215522">-</p>
</td>
<td class="cellrowborder" valign="top" width="36.22362236223623%" headers="mcps1.2.7.1.2 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p195481842105215">Size of **fftsAddr** data.</p>
<p id="p195766538497">This parameter is mandatory when <span class="parmvalue" id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_parmvalue111181527191815">**param_type**</span> is set to **fftsAddr**.</p>
</td>
<td class="cellrowborder" valign="top" width="10.09100910091009%" headers="mcps1.2.7.1.3 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p63579615159">int</p>
</td>
<td class="cellrowborder" valign="top" width="6.950695069506951%" headers="mcps1.2.7.1.4 "><p id="zh-cn_topic_0000002015877333_zh-cn_topic_0000001752612702_p1766051391419">No</p>
</td>
</tr>
</tbody>
</table>

> [!NOTICE]NOTICE
> 
> - The number of parameter values in **output** must be the same as that in **input**. Otherwise, test case generation fails.
>    For example, if **input** supports two types, **output** must also support two types.
>    Similarly, the number of values of **type**, **shape**, or **value\_range** in each **input** or **output** must be the same.
> - The number of parameter values in each **input** of an operator must be the same. Otherwise, test case generation fails.
>    The number of values of **type**, **shape**, and **value\_range** in each **input** must be the same.

## mstx Extended Functions

**mstx API Overview**

MindStudio provides the mstx profiling API, which enables users to embed custom markers within their applications. These markers allow for the precise identification of critical code segments during performance analysis. For details, see [**Table 1** C/C++ mstx API List](#c-mstx-api-list) and [**Table 2** Python mstx API List](#python-mstx-api-list). For further details about the API usage, see *MindStudio mstx API Reference*.

**Table 1** C/C++ mstx API List<a id="c-mstx-api-list"></a>

|API|Description|msOpProf Support|
|---|---|---|
|mstxRangeStartA|Marks the beginning of a specific mstx range.|Supported|
|mstxRangeEnd|Marks the end of a specific mstx range.|Supported|

**Table 2** Python mstx API List<a id="python-mstx-api-list"></a>

|API|Description|msOpProf Support|
|---|---|---|
|mstx.range_start|Marks the beginning of a specific mstx range.|Supported|
|mstx.range_end|Marks the end of a specific mstx range.|Supported|

**mstx API Usage**

- msOpProf allows users to use the **mstx** API to tune specific operators, customize the start time and end time of the code segment or specified key functions, identify key functions or computing APIs, and quickly demarcate performance issues.
- The mstx API is disabled by default. If the mstx API is called in the application, the mstx instrumentation function is enabled based on the actual application scenario. For example, the **--mstx=on** flag enables mstx APIs within the user program, while **--mstx-include** can be used to target specific mstx APIs. For detailed usage, refer to the **--mstx** and **--mstx-include** parameters in the "Command Reference" sections of the [*msopprof User Guide*](./msopprof_user_guide.md#command-reference) and the [*msopprof Simulator Mode User Guide*](./msopprof_simulator_user_guide.md#command-reference).
- The mstx API can be used via library files or header files. An implementation example can be found at this [sample](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation):

    > [!NOTE]NOTE
    > 
    > - This sample project does not support <term>Atlas A3 training products</term>.
    > - Replace $**\{INSTALL\_DIR\}** with the file storage path after CANN is installed. For example, if the installation is performed by the **root** user, the default file storage path is **/usr/local/Ascend/cann**.

    - Add the `libms_tools_ext.so` library file located at `${INSTALL_DIR}/lib64/libms_tools_ext.so` to the `CMakeLists.txt` file at `${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/CMakeLists.txt`.

        ```shell
        # Header path
        include_directories(
             ...
            ${CUST_PKG_PATH}/include
        )
        ...
        target_link_libraries( 
            ...
            dl
        )
        ```

    - In the `main.cpp` file at `${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/main.cpp`, compile and link the user program with the `dl` library. The corresponding header file `ms_tools_ext.h` is located at `${INSTALL_DIR}/include/mstx`.

        ```shell
        ...
        #include "mstx/ms_tools_ext.h"
        ...
        ```

**Example**

After msOpProf is started, run the `msprof op --mstx=on --mstx-include=range1 --launch-count=2 python cal.py` command. This command will profile the operators defined within the `range1` scope, specifically the `sub` and `mul` operators.

```python
import mstx
import torch
import torch_npu
 
x = torch.Tensor([1,2,3,4]).npu()
y = torch.Tensor([1,2,3,4]).npu()

a = x + y
range1_id = mstx.range_start("range1", None)
b = a - x
c = a * x
mstx.range_end(range1_id)
range2_id = mstx.range_start("range2", None)
d = x / y
range3_id = mstx.range_start("range3", None)
e = torch.abs(y)
mstx.range_end(range3_id)
f = x + e
mstx.range_end(range2_id)
```
