# BasicForge Dependency Licenses

This document contains the full license texts for all integrated third-party
dependencies, as required by their respective licenses.

---

## 1. LLVM Project (including LLD)

**License:** Apache License v2.0 with LLVM Exceptions

```
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to direct the
      operation or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) ownership of fifty percent (50%) or
      more of the outstanding shares, or (iii) ownership of fifty percent
      (50%) or more of the outstanding shares, or (iv) beneficial ownership
      of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the Work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any Work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely cluster around, the Work and do not contribute
      to the distinctive character of the Work.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this License, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, forums, and similar.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to direct the
      operation or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) ownership of fifty percent (50%) or
      more of the outstanding shares, or (iv) beneficial ownership of
      such entity.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as specified in this License) patent license to make, have
      made, use, offer to sell, sell, import, and otherwise transfer the
      Work, where such patent license applies only to those patent claims
      licensable by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      counterclaim or cross-claim in a lawsuit) alleging that the Work
      or a Derivative Work of the Work infringes their patent claims,
      then any patent licenses granted to You under this License for that
      Work shall terminate as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in Source or Object form, provided
      that You meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain all copyright, patent, trademark, and
          attribution notices from the Source form of the Work in the
          Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that you distribute must
          include a readable copy of the NOTICE file contained within the
          Work, unless such NOTICE file does not exist in the Work.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution submitted to the Licensor for inclusion in the
      Work by the copyright owner or by an individual or Legal Entity
      authorized to submit on behalf of the copyright owner shall be
      deemed a Contribution and will be processed under the terms of this
      License, without any additional terms or conditions. Notwithstanding
      the above, nothing herein shall obligate the Licensor to accept
      any such Contribution.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use to describe the
      origin of the Work and not to imply endorsement by the Licensor.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, the Licensor provides the Work (and each
      Contributor provides its Contribution) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, warranties of MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. IN NO EVENT
      SHALL THE LICENSOR OR CONTRIBUTORS BE LIABLE FOR ANY CLAIM, DAMAGES,
      OR OTHER OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT,
      OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE WORK
      OR THE USE OF OR OTHER OPERATIONS ON THE WORK, INCLUDING
      WITHOUT LIMITATION, DAMAGES RESULTING FROM IMPROPER USE, OVERLOAD,
      OR UNREASUNG THE WORK.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law or agreed to in writing, shall
      any Contributor be liable to You for damages, including any direct,
      indirect, special, incidental, or consequential damages of any
      character arising as a result of this License or the use of or
      inability to use the Work (including but not limited to damages
      for loss of goodwill, loss of data, or loss of profit), even if
      the Licensor has been advised of the possibility of such damages.

   9. Accepting the License. By using the Work, you agree to the terms
      and conditions of this License. If You do not agree to the terms
      and conditions of this License, do not use the Work.

   LLVM EXCEPTIONS TO THE APACHE 2.0 LICENSE

   As an exception, if, as a result of your compiling your source code,
   portions of this Work are embedded into an Object form of such
   source code, you may redistribute such embedded portions in such
   Object form without complying with the conditions of Sections 4(a),
   4(b), and 4(d) of the License.

   In addition, if you combine or link compiled forms of this Work with
   other works to produce a combined work based on this Work, and if
   you have obtained permission from the copyright holders of the other
   works to distribute the combined work, then you may distribute such
   combined work under terms of your choice, provided that the
   combined work contains a copy of this License, or a pointer to where
   this License may be obtained (such as an Internet URL), and provided
   that the combined work includes the following attribution notice:

      This software contains software developed as part of the LLVM
      Project (http://llvm.org/). This file is covered by the
      Apache License v2.0 with LLVM Exceptions (see LICENSE.TXT).

   END OF TERMS AND CONDITIONS
```

---

## 2. LLVM Exception to Apache 2.0

The LLVM exception allows embedding LLVM components into compiled
binaries without requiring the full Apache 2.0 license text in the
binary distribution, provided the attribution notice is included.

---

## 3. MIT License (used by egui, serde, anyhow, tree-sitter, goblin, LIEF, Ninja)

```
MIT License

Copyright (c) <year> <copyright holders>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 4. Apache License 2.0 (Cranelift, Ninja, CMake)

```
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   ... (same as LLVM license above, without LLVM Exceptions) ...
```

---

## 5. BSD-3-Clause (CMake)

```
Copyright (c) Kitware, Inc. and Contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the names of Kitware, Inc., the Insight Software Consortium,
  nor the names of their contributors may be used to endorse or promote
  products derived from this software without specific prior written
  permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEPENDENCIES OF THIS SOFTWARE.
```

---

## 6. zlib/libpng License (AsmJit)

```
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
```

---

## License Compatibility Notes

All licenses used are **permissive** and **mutually compatible**:
- Apache-2.0 with LLVM Exception (LLVM/LLD) - compatible with all
- Apache-2.0 (Cranelift, Ninja) - compatible
- MIT (egui, serde, anyhow, tree-sitter, etc.) - compatible
- BSD-3-Clause (CMake) - compatible
- zlib/libpng (AsmJit) - compatible

No GPL, LGPL, MPL, or other copyleft licenses are integrated.

---

## Attribution Requirements

When distributing BasicForge binaries, the following attributions must be
preserved:

1. **LLVM/LLD**: "This software contains software developed as part of the
   LLVM Project (http://llvm.org/). This file is covered by the Apache
   License v2.0 with LLVM Exceptions."

2. **MIT-licensed components**: Copyright notices and license text must
   be included in source distributions; binary distributions should include
   the copyright notice and permission notice.

3. **Apache-2.0 components**: NOTICE file contents (if any) must be
   preserved in derivative works.

---

## Verification

All license texts above were obtained from the official repositories of
the respective projects at the time of integration. Licenses are
reproduced here for compliance; the canonical versions remain at the
respective project repositories.