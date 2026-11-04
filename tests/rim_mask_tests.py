"""Focused categorical rim-coverage and motion-blur regressions."""
import sys, unittest
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools/car_pipeline/pipeline'))
from rim_masks import material_mask, blur_mask

class RimMasks(unittest.TestCase):
    def test_protected_materials_and_ambiguous_edges(self):
        # IDs: rim=1, tyre=2, brake=3, badge=4. All colours deliberately white:
        # classification must come from identity rather than brightness.
        ids=np.zeros((512,512,4),np.uint8);raw=np.full_like(ids,255)
        for column,material in enumerate([1,2,3,4]): ids[:4,column*4:column*4+4]=[material,material,material,255]
        ids[:4,16:20]=[2,2,2,255];ids[:2,16:20]=[1,1,1,255] # ambiguous rim/tyre tie
        ids[:4,20:24]=[1,1,1,255] # invisible final sprite pixel
        ids[:4,24:28]=[1,1,1,0];raw[:4,24:28,3]=0
        alpha=np.full((128,128),255,np.uint8);alpha[0,5]=0
        result=material_mask(ids,raw,[1],alpha)
        self.assertEqual(result[0,:7].tolist(),[255,0,0,0,0,0,0])
    def test_blur_coverage_and_permanently_protected_pixels(self):
        masks=np.zeros((8,2,3),np.uint8);masks[:,0,0]=255;masks[:4,0,1]=255;masks[:,0,2]=255
        coverage=np.ones((8,2,3),bool);alpha=np.full((2,3),255,np.uint8);alpha[0,2]=0
        result=blur_mask(masks,coverage,alpha)
        self.assertEqual(result[0].tolist(),[255,128,0])
        self.assertTrue(np.all(result[1]==0))
        coverage[4:,0,1]=False
        self.assertEqual(blur_mask(masks,coverage,alpha)[0,1],255)
if __name__=='__main__': unittest.main()
